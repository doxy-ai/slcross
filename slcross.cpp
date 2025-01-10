#define SLCROSS_IMPLEMENTATION
#include "slcross.hpp"
#include "cstring_from_view.hpp"

#include <spirv-tools/optimizer.hpp>
#include <spirv-tools/linker.hpp>
#include <spirv-tools/libspirv.hpp>

#include <spirv_glsl.hpp>
#include <spirv_hlsl.hpp>
#include <spirv_msl.hpp>

#ifdef SLCROSS_ENABLE_READING_GLSL
	#include <glslang/Public/ShaderLang.h>
	#include <glslang/Public/ResourceLimits.h>
	#include <SPIRV/GlslangToSpv.h>
#endif // SLCROSS_ENABLE_READING_GLSL

#ifdef SLCROSS_ENABLE_SLANG
	#include <slang-com-ptr.h>
	#include <slang.h>
	#include <core/slang-string-util.h>
#endif // SLCROSS_ENABLE_SLANG

#ifdef SLCROSS_ENABLE_WGSL
	#include <tint/tint.h>
#endif // SLCROSS_ENABLE_WGSL

namespace slcross {

	std::optional<std::string> validate(spirv_view module, bool throw_on_error /* = true */) {
		spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_3);
		std::ostringstream s;
		tools.SetMessageConsumer([&s](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) {
			s << position.line << ":" << position.column << " error: " << message << "\n";
		});

		if(tools.Validate(module.data(), module.size())) return {};
		if(throw_on_error) throw error(s.str());
		return s.str();
	}

	spirv optimize(spirv_view unoptimized, bool for_speed /*= true*/)  {
		spvtools::Optimizer opt(SPV_ENV_VULKAN_1_3);
		std::ostringstream s;
		opt.SetMessageConsumer([&s](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) {
			s << position.line << ":" << position.column << " error: " << message << "\n";
		});
		if(for_speed) opt.RegisterPerformancePasses();
		else opt.RegisterSizePasses();

		std::vector<uint32_t> optimized;
		auto res = opt.Run(unoptimized.data(), unoptimized.size(), &optimized);
		if(!res) throw error(s.str());
		return optimized;
	}

	spirv link(std::vector<std::vector<uint32_t>> binaries) {
		spvtools::Context ctx(SPV_ENV_VULKAN_1_3);
		std::ostringstream s;
		ctx.SetMessageConsumer([&s](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) {
			s << position.line << ":" << position.column << " error: " << message << "\n";
		});

		std::vector<uint32_t> linked;
		auto res = spvtools::Link(ctx, binaries, &linked);
		if(res != SPV_SUCCESS) throw error(s.str());
		return linked;
	}

	std::string disassemble(spirv_view spirv) {
		spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_3);
		std::string out;
		tools.Disassemble(spirv.data(), spirv.size(), &out);
		return out;
	}

	namespace glsl {
#ifdef SLCROSS_ENABLE_READING_GLSL
		namespace detail {
			inline void maybe_initialize_glslang() {
				static bool glslang_initialized = false;
				if(!glslang_initialized) {
					glslang::InitializeProcess();
					glslang_initialized = true;
				}
			}

			inline EShLanguage to_glslang(shader_stage stage) {
				switch(stage){
				case shader_stage::Vertex: return EShLangVertex;
				case shader_stage::TesselationControl: return EShLangTessControl;
				case shader_stage::TesselationEvaluation: return EShLangTessEvaluation;
				case shader_stage::Geometry: return EShLangGeometry;
				case shader_stage::Fragment: return EShLangFragment;
				case shader_stage::Compute: return EShLangCompute;
				case shader_stage::RayGen: return EShLangRayGen;
				case shader_stage::Intersect: return EShLangIntersect;
				case shader_stage::AnyHit: return EShLangAnyHit;
				case shader_stage::ClosestHit: return EShLangClosestHit;
				case shader_stage::Miss: return EShLangMiss;
				case shader_stage::Callable: return EShLangCallable;
				case shader_stage::Task: return EShLangTask;
				case shader_stage::Mesh: return EShLangMesh;
				default: return EShLangCompute;
				}
			}

			inline spv::ExecutionModel to_cross(shader_stage stage) {
				switch(stage){
				case shader_stage::Vertex: return spv::ExecutionModel::ExecutionModelVertex;
				case shader_stage::TesselationControl: return spv::ExecutionModel::ExecutionModelTessellationControl;
				case shader_stage::TesselationEvaluation: return spv::ExecutionModel::ExecutionModelTessellationEvaluation;
				case shader_stage::Geometry: return spv::ExecutionModel::ExecutionModelGeometry;
				case shader_stage::Fragment: return spv::ExecutionModel::ExecutionModelFragment;
				case shader_stage::Compute: return spv::ExecutionModel::ExecutionModelGLCompute;
				case shader_stage::RayGen: return spv::ExecutionModel::ExecutionModelRayGenerationKHR;
				case shader_stage::Intersect: return spv::ExecutionModel::ExecutionModelIntersectionKHR;
				case shader_stage::AnyHit: return spv::ExecutionModel::ExecutionModelAnyHitKHR;
				case shader_stage::ClosestHit: return spv::ExecutionModel::ExecutionModelClosestHitKHR;
				case shader_stage::Miss: return spv::ExecutionModel::ExecutionModelMissKHR;
				case shader_stage::Callable: return spv::ExecutionModel::ExecutionModelCallableKHR;
				case shader_stage::Task: return spv::ExecutionModel::ExecutionModelTaskEXT;
				case shader_stage::Mesh: return spv::ExecutionModel::ExecutionModelMeshEXT;
				default: return spv::ExecutionModel::ExecutionModelGLCompute;
				}
			}
		}

		spirv parse_from_memory(
			shader_stage stage,
			const std::string_view content,
			const std::string_view entry_point_ /*= "main"*/,
			client_version version /*= spirv_version::Vulkan_1_3*/
			// glslang::EShTargetClientVersion version = glslang::EShTargetClientVersion::EShTargetVulkan_1_3
			// const std::string& path = "generated.glsl"
		) {
			detail::maybe_initialize_glslang();
			auto client = version == client_version::OpenGL_450 ? glslang::EShClient::EShClientOpenGL : glslang::EShClient::EShClientVulkan;

			const char* shaderSource = cstring_from_view(content);
			const char* entry_point = cstring_from_view<1>(entry_point_);
			glslang::TShader shader(detail::to_glslang(stage));
			shader.setEnvInput(glslang::EShSource::EShSourceGlsl, detail::to_glslang(stage), client, 100);
			shader.setEnvClient(client, (glslang::EShTargetClientVersion)version);
			shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_3);
			shader.setEntryPoint(entry_point);
			shader.setSourceEntryPoint(entry_point);
			shader.setStrings(&shaderSource, 1);

			if (!shader.parse(GetDefaultResources(), 100, false, EShMsgDefault))
				throw error(shader.getInfoLog());

			glslang::TProgram program;
			program.addShader(&shader);

			if (!program.link(EShMsgDefault)) throw error(program.getInfoLog());

			std::vector<uint32_t> spirv;
			glslang::GlslangToSpv(*program.getIntermediate(detail::to_glslang(stage)), spirv);
			return spirv;
		}
#endif // SLCROSS_ENABLE_READING_GLSL

		std::string generate(spirv_view module, shader_stage stage, std::string_view entry_point /* = "main" */, bool target_vulkan /* = true */, bool target_web /* = false */, uint32_t version /* = 450 */) {
			spirv_cross::CompilerGLSL glsl(module.data(), module.size());
			glsl.set_common_options({.version = version, .es = target_web, .vulkan_semantics = target_vulkan});
			glsl.set_entry_point(std::string{entry_point}, detail::to_cross(stage));
			return glsl.compile();
		}
	}

	namespace hlsl {
		std::string generate(spirv_view module, uint32_t shader_model /* = 30 */) {
			spirv_cross::CompilerHLSL hlsl(module.data(), module.size());
			hlsl.set_hlsl_options({.shader_model = shader_model});
			return hlsl.compile();
		}
	}

	namespace msl {
		std::string generate(spirv_view module, bool target_IOS /* = false */) {
			spirv_cross::CompilerMSL msl(module.data(), module.size());
			msl.set_msl_options({.platform = target_IOS ? spirv_cross::CompilerMSL::Options::iOS : spirv_cross::CompilerMSL::Options::macOS});
			return msl.compile();
		}
	}

#ifdef SLCROSS_ENABLE_SLANG
	#define ASSERT_ON_FAIL(x) \
    {                           \
        SlangResult _res = (x); \
        if (SLANG_FAILED(_res)) \
        {                       \
            assert(false);      \
            return {};          \
        }                       \
    }

	namespace slang {
		namespace detail {
			::slang::IGlobalSession* maybe_create_global_session() {
				static Slang::ComPtr<::slang::IGlobalSession> global_session = []() -> Slang::ComPtr<::slang::IGlobalSession>{
					Slang::ComPtr<::slang::IGlobalSession> slangGlobalSession;
					SLANG_RETURN_NULL_ON_FAIL(::slang::createGlobalSession(slangGlobalSession.writeRef()));
					return slangGlobalSession;
				}();
				return global_session.get();
			}
		}

		struct session: public ::slang::ISession { using ::slang::ISession::ISession; };

		session* create_session() {
			// First we need to create slang global session with work with the Slang API.
			auto globalSession = detail::maybe_create_global_session();

			::slang::SessionDesc sessionDesc = {};
			::slang::TargetDesc targetDesc = {};
			targetDesc.format = SLANG_SPIRV;
			targetDesc.profile = globalSession->findProfile("spirv_1_5");
			targetDesc.flags = 0;

			sessionDesc.targets = &targetDesc;
			sessionDesc.targetCount = 1;

			std::vector<::slang::CompilerOptionEntry> options;
			options.push_back(
				{::slang::CompilerOptionName::Optimization,
				{::slang::CompilerOptionValueKind::Int, 0, 0, nullptr, nullptr}});
			options.push_back(
				{::slang::CompilerOptionName::DebugInformation,
				{::slang::CompilerOptionValueKind::Int, 2, 0, nullptr, nullptr}});
			options.push_back(
				{::slang::CompilerOptionName::EmitSpirvDirectly,
				{::slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}});
			options.push_back(
				{::slang::CompilerOptionName::VulkanEmitReflection,
				{::slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}});
			options.push_back(
				{::slang::CompilerOptionName::SkipSPIRVValidation,
				{::slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}});
			// options.push_back(
			// 	{::slang::CompilerOptionName::VulkanUseGLLayout,
			// 	{::slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}});
			// options.push_back(
			// 	{::slang::CompilerOptionName::VulkanUseEntryPointName,
			// 	{::slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}});
			// options.push_back(
			// 	{::slang::CompilerOptionName::ValidateIr,
			// 	{::slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}});
			sessionDesc.compilerOptionEntries = options.data();
			sessionDesc.compilerOptionEntryCount = options.size();

			session* out;
			ASSERT_ON_FAIL(globalSession->createSession(sessionDesc, (::slang::ISession**)&out));
			return out;
		}

		void free_session(session* session) {
			auto free = Slang::ComPtr<::slang::ISession>{session};
		}


		void report_slang_diagnostic(const Slang::ComPtr<::slang::IBlob>& diagnosticBlob) {
			if(diagnosticBlob != nullptr) {
				std::string str = (char*)diagnosticBlob->getBufferPointer();
				if(str.find("error") != std::string::npos && str.find("error 100") == std::string::npos) // Throw on errors (except error 100)
					throw error(str);
				else fprintf(stderr, "%s\n", str.c_str());
			}
		}

		bool inject_module_from_memory(session* session, std::string_view content, std::string_view path /* = "generated.slang" */, std::string_view module /* = "generated" */) {
			assert(session);
			auto dbg = session->getLoadedModuleCount();
			::slang::IModule* slangModule = nullptr;
			{
				Slang::ComPtr<::slang::IBlob> diagnosticBlob;
				slangModule = session->loadModuleFromSourceString(cstring_from_view<0>(module), cstring_from_view<1>(path), cstring_from_view<2>(content), diagnosticBlob.writeRef());
				report_slang_diagnostic(diagnosticBlob);
				if (!slangModule)
					throw error("Failed to load slang module!");
			}

			Slang::List<::slang::IComponentType*> componentTypes;
			componentTypes.add(slangModule);

			Slang::ComPtr<::slang::IComponentType> composedProgram;
			{
				Slang::ComPtr<::slang::IBlob> diagnosticBlob;
				SlangResult result = session->createCompositeComponentType(
					componentTypes.getBuffer(),
					componentTypes.getCount(),
					composedProgram.writeRef(),
					diagnosticBlob.writeRef());
				report_slang_diagnostic(diagnosticBlob);
				ASSERT_ON_FAIL(result);
			}

			return true;
		}

		spirv parse_from_memory(session* session_, std::string_view content, std::string_view entry_point /* = "main" */, std::string_view path /* = "generated.slang" */, std::string_view module /* = "generated" */) {
			Slang::ComPtr<::slang::ISession> lifetimeTrackedSession;
			::slang::ISession* session;
			if(session_) session = session_;
			else (lifetimeTrackedSession = create_session(), session = lifetimeTrackedSession.get());

			Slang::ComPtr<::slang::IModule> slangModule;
			{
				Slang::ComPtr<::slang::IBlob> diagnosticBlob;
				slangModule = session->loadModuleFromSourceString(cstring_from_view<0>(module), cstring_from_view<1>(path), cstring_from_view<2>(content), diagnosticBlob.writeRef());
				report_slang_diagnostic(diagnosticBlob);
				if (!slangModule)
					throw error("Failed to load slang module!");
			}

			Slang::ComPtr<::slang::IComponentType> linkedProgram;
			{
				Slang::ComPtr<::slang::IBlob> diagnosticBlob;
				slangModule->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());
				report_slang_diagnostic(diagnosticBlob);
				if (!linkedProgram)
					throw error("Failed to link slang program!");
			}

			Slang::ComPtr<::slang::IEntryPoint> entryPoint;
			slangModule->findEntryPointByName(cstring_from_view<4>(entry_point), entryPoint.writeRef());
			if (!entryPoint)
				throw error("Failed to find entry point: " + std::string(entry_point));


			Slang::List<::slang::IComponentType*> componentTypes;
			componentTypes.add(linkedProgram);
			componentTypes.add(entryPoint);

			Slang::ComPtr<::slang::IComponentType> composedProgram;
			{
				Slang::ComPtr<::slang::IBlob> diagnosticBlob;
				SlangResult result = session->createCompositeComponentType(
					componentTypes.getBuffer(),
					componentTypes.getCount(),
					composedProgram.writeRef(),
					diagnosticBlob.writeRef());
				report_slang_diagnostic(diagnosticBlob);
				ASSERT_ON_FAIL(result);
			}

			Slang::ComPtr<::slang::IBlob> spirvCode;
			{
				Slang::ComPtr<::slang::IBlob> diagnosticBlob;
				SlangResult result = composedProgram->getEntryPointCode(
					0,
					0,
					spirvCode.writeRef(),
					diagnosticBlob.writeRef());
				report_slang_diagnostic(diagnosticBlob);
				// if(diagnosticBlob != nullptr)
				// 	// NOTE: There is always a library not found error! So we don't throw an exception...
				// 	fprintf(stderr, "%s\n", (char*)diagnosticBlob->getBufferPointer());
				ASSERT_ON_FAIL(result);
			}

			auto view = spirv_view{static_cast<const uint32_t*>(spirvCode->getBufferPointer()), spirvCode->getBufferSize()/ sizeof(uint32_t)}; // TODO: Need to divide size by 4?
			return spirv{view.begin(), view.end()};
		}
	}
#endif // SLCROSS_ENABLE_SLANG

#ifdef SLCROSS_ENABLE_WGSL
	namespace wgsl {
		namespace detail {
			inline void maybe_initialize_tint() {
				static bool tint_initialized = false;
				if(!tint_initialized) {
					tint::Initialize();
					tint_initialized = true;
				}
			}
		}

		spirv parse_from_memory(const std::string_view content, bool target_vulkan /*= true*/, const std::string_view path /*= "generated.wgsl"*/) {
			detail::maybe_initialize_tint();

			tint::Source::File file(std::string{path}, std::string{content});
			auto module = tint::wgsl::reader::WgslToIR(&file, {.allowed_features = tint::wgsl::AllowedFeatures::Everything()});
			if(module != tint::Success)
				throw error(module.Failure().reason.Str());

			auto spirv = tint::spirv::writer::Generate(module.Get(), {.use_vulkan_memory_model = target_vulkan});
			if(spirv != tint::Success)
				throw error(spirv.Failure().reason.Str());
			return spirv->spirv;
		}

		std::string generate(spirv_view module) {
			detail::maybe_initialize_tint();

			auto tint = tint::spirv::reader::Read(spirv{module.begin(), module.end()}, {.allowed_features = tint::wgsl::AllowedFeatures::Everything()});

			auto output = tint::wgsl::writer::Generate(tint, {});
			if(output != tint::Success)
				throw slcross::error(output.Failure().reason.Str());
			return output->wgsl;
		}
	}
#endif // SLCROSS_ENABLE_WGSL
}