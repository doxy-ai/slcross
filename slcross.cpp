#define SLCROSS_IMPLEMENTATION
#include "slcross.hpp"
#include "cstring_from_view.hpp"

#include <tint/tint.h>

#include <spirv-tools/optimizer.hpp>
#include <spirv-tools/linker.hpp>
#include <spirv-tools/libspirv.hpp>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <spirv_glsl.hpp>
#include <spirv_hlsl.hpp>
#include <spirv_msl.hpp>

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

	namespace glsl {
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
				default: return EShLangVertex;
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

		std::string generate(spirv_view module, bool target_vulkan /* = true */, bool target_web /* = false */, uint32_t version /* = 450 */) {
			spirv_cross::CompilerGLSL glsl(module.data(), module.size());
			glsl.set_common_options({.version = version, .es = target_web, .vulkan_semantics = target_vulkan});
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
}