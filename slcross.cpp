#include <slang.h>

#define SLCROSS_IMPLEMENTATION
#include "slcross.hpp"

#include "cstring_from_view.hpp"
#include "thirdparty/slang/source/core/slang-list.h"

#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <spirv_hlsl.hpp>

namespace slcross {

	namespace detail {
		void report_slang_diagnostic(const Slang::ComPtr<::slang::IBlob>& diagnosticBlob) {
			if(diagnosticBlob != nullptr) {
				std::string str = (char*)diagnosticBlob->getBufferPointer();
				if(str.find("error") != std::string::npos && str.find("error 100") == std::string::npos) // Throw on errors (except error 100)
					throw std::runtime_error(str);
				else fprintf(stderr, "%s\n", str.c_str());
			}
		}

		#define ASSERT_ON_FAIL(x) \
		{                           \
			SlangResult _res = (x); \
			if (SLANG_FAILED(_res)) \
			{                       \
				assert(false);      \
				return {};          \
			}                       \
		}
	}

	
	::slang::IGlobalSession* session::maybe_create_global_session() {
		static Slang::ComPtr<::slang::IGlobalSession> global_session = []() -> Slang::ComPtr<::slang::IGlobalSession>{
			Slang::ComPtr<::slang::IGlobalSession> slangGlobalSession;
			SLANG_RETURN_NULL_ON_FAIL(::slang::createGlobalSession(slangGlobalSession.writeRef()));
			return slangGlobalSession;
		}();
		return global_session.get();
	}

	session session::create(std::span<const ::slang::CompilerOptionEntry> options /* = {} */) {
		auto gs = maybe_create_global_session();

		std::array<::slang::TargetDesc, 5> targets = {
			::slang::TargetDesc{.format = SLANG_SPIRV, .profile = gs->findProfile("spirv_1_6"), .flags = 0},
			::slang::TargetDesc{.format = SLANG_GLSL, /* .profile = gs->findProfile("spirv_1_6"), */ .flags = 0},
			::slang::TargetDesc{.format = SLANG_HLSL, /* .profile = gs->findProfile("spirv_1_6"), */ .flags = 0},
			::slang::TargetDesc{.format = SLANG_METAL, /* .profile = gs->findProfile("spirv_1_6"), */ .flags = 0},
			::slang::TargetDesc{.format = SLANG_WGSL, /* .profile = gs->findProfile("spirv_1_6"), */ .flags = 0}
		};

		::slang::SessionDesc session = {};
		session.targets = targets.data();
		session.targetCount = targets.size();
		session.compilerOptionEntries = const_cast<::slang::CompilerOptionEntry*>(options.data());
		session.compilerOptionEntryCount = options.size();

		struct session out;
		ASSERT_ON_FAIL(gs->createSession(session, (::slang::ISession**)&out.pointer));
		return out;
	}

	Slang::ComPtr<::slang::IComponentType> session::compose_program(std::string_view entry_point /* = "main" */) {
		Slang::ComPtr<::slang::IModule> slangModule;
		Slang::ComPtr<::slang::IEntryPoint> foundEntryPoint;
		for (size_t i = 0; i < pointer->getLoadedModuleCount(); ++i) {
			Slang::ComPtr<::slang::IModule> tmp;
			tmp = pointer->getLoadedModule(i);
			if (!tmp) continue;

			tmp->findEntryPointByName(cstring_from_view<0>(entry_point), foundEntryPoint.writeRef());
			if (!foundEntryPoint) continue;

			slangModule = std::move(tmp);
			break; // We found a module that works!
		}

		if(!slangModule)
			throw std::runtime_error("Failed to find all entry points in a loaded module!");

		Slang::ComPtr<::slang::IComponentType> linkedProgram;
		{
			Slang::ComPtr<::slang::IBlob> diagnosticBlob;
			slangModule->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());
			detail::report_slang_diagnostic(diagnosticBlob);
			if (!linkedProgram)
				throw std::runtime_error("Failed to link slang program!");
		}
		
		Slang::List<::slang::IComponentType*> componentTypes;
		componentTypes.add(linkedProgram);
		componentTypes.add(foundEntryPoint);

		Slang::ComPtr<::slang::IComponentType> composedProgram;
		{
			Slang::ComPtr<::slang::IBlob> diagnosticBlob;
			SlangResult result = pointer->createCompositeComponentType(
				componentTypes.getBuffer(),
				componentTypes.getCount(),
				composedProgram.writeRef(),
				diagnosticBlob.writeRef());
			detail::report_slang_diagnostic(diagnosticBlob);
			ASSERT_ON_FAIL(result);
		}

		return composedProgram;
	}

	Slang::ComPtr<::slang::IBlob> session::generate_code(size_t target_index, std::string_view entry_point /* = "main" */) {
		assert(pointer);
		auto composedProgram = compose_program(entry_point);

		Slang::ComPtr<::slang::IBlob> out;
		{
			Slang::ComPtr<::slang::IBlob> diagnosticBlob;
			SlangResult result = composedProgram->getEntryPointCode(
				0,
				target_index,
				out.writeRef(),
				diagnosticBlob.writeRef());
			static bool first = true;
			if(!first) // The first time it will complain about some missing irrelevant libraries
				detail::report_slang_diagnostic(diagnosticBlob);
			else first = false;
			ASSERT_ON_FAIL(result);
		}

		return out;
	}

	void session::free() {
		pointer->Release();
	}

	namespace slang {
		session parse_from_memory(std::string_view source, std::string_view path /* = "generated.slang" */, std::string_view module /* = "generated" */, std::optional<session> existing_session /* = {} */) {
			struct session session_ = existing_session ? existing_session.value() : session::create();
			auto session = session_.pointer;
			assert(session);

			::slang::IModule* slangModule = nullptr;
			{
				Slang::ComPtr<::slang::IBlob> diagnosticBlob;
				slangModule = session->loadModuleFromSourceString(cstring_from_view<0>(module), cstring_from_view<1>(path), cstring_from_view<2>(source), diagnosticBlob.writeRef());
				detail::report_slang_diagnostic(diagnosticBlob);
				if (!slangModule)
					return {};
			}

			return {session};
		}
	}

	namespace spirv {
		std::vector<uint32_t> generate(session session, std::string_view entry_point /* = "main" */) {
			constexpr static size_t spirv_index = 0;
			Slang::ComPtr<::slang::IBlob> code = session.generate_code(spirv_index, entry_point);

			auto view = std::span<const uint32_t>{static_cast<const uint32_t*>(code->getBufferPointer()), code->getBufferSize()/ sizeof(uint32_t)};
			return std::vector<uint32_t>{view.begin(), view.end()};
		}
	}

	namespace glsl {
		std::string generate(session session, std::string_view entry_point /* = "main" */) {
			constexpr static size_t glsl_index = 1;
			Slang::ComPtr<::slang::IBlob> code = session.generate_code(glsl_index, entry_point);
			
			auto view = std::string_view{static_cast<const char*>(code->getBufferPointer()), code->getBufferSize()};
			return std::string(view);
		}
	}

	namespace hlsl {
		std::string generate(session session, std::string_view entry_point /* = "main" */) {
			constexpr static size_t hlsl_index = 2;
			Slang::ComPtr<::slang::IBlob> code = session.generate_code(hlsl_index, entry_point);
			
			auto view = std::string_view{static_cast<const char*>(code->getBufferPointer()), code->getBufferSize()};
			return std::string(view);
		}
	}

	namespace msl {
		std::string generate(session session, std::string_view entry_point /* = "main" */) {
			constexpr static size_t msl_index = 3;
			Slang::ComPtr<::slang::IBlob> code = session.generate_code(msl_index, entry_point);
			
			auto view = std::string_view{static_cast<const char*>(code->getBufferPointer()), code->getBufferSize()};
			return std::string(view);
		}
	}

	namespace wgsl {
		std::string generate(session session, std::string_view entry_point /* = "main" */) {
			constexpr static size_t wgsl_index = 4;
			Slang::ComPtr<::slang::IBlob> code = session.generate_code(wgsl_index, entry_point);
			
			auto view = std::string_view{static_cast<const char*>(code->getBufferPointer()), code->getBufferSize()};
			return std::string(view);
		}
	}

	namespace experimental {
		namespace spirv {
			session parse_from_memory(std::span<const uint32_t> spirv, std::map<shader_stage, std::string_view> entry_points, std::string_view path /* = "generated.slang" */, std::string_view module /* = "generated" */, std::optional<session> existing_session /* = {} */) {
				spirv_cross::CompilerHLSL compiler(spirv.data(), spirv.size());
				compiler.set_hlsl_options({.shader_model = 60}); // Slang apparently supports up through 6.0
				auto hlsl = compiler.compile();
				for(auto [stage, name]: entry_points) {
					auto namePos = hlsl.find(" " + std::string(name));
					auto line = hlsl.rfind("\n", namePos);
					hlsl = hlsl.substr(0, line) + "\n[shader(\"" + magic_enum::enum_name(stage).data() + "\")]" + hlsl.substr(line);
				}
				std::cout << hlsl << std::endl;
				return slang::parse_from_memory(hlsl, path, module, existing_session);
			}
		}
	}
}