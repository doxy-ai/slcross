#include "preprocessor.hpp"

#include <concepts>
#include <optional>
#include <span>
#include <vector>

namespace slcross {

	using spirv = std::vector<uint32_t>;
	using spirv_view = std::span<const uint32_t>;

	enum class client_version {
		Vulkan_1_0 = (1 << 22),                  // Vulkan 1.0
		Vulkan_1_1 = (1 << 22) | (1 << 12),      // Vulkan 1.1
		Vulkan_1_2 = (1 << 22) | (2 << 12),      // Vulkan 1.2
		Vulkan_1_3 = (1 << 22) | (3 << 12),      // Vulkan 1.3
		OpenGL_450 = 450,                        // OpenGL
	};

	std::optional<std::string> validate(spirv_view module, bool throw_on_error = true);

	spirv optimize(spirv_view unoptimized, bool for_speed = true);

	spirv link(std::vector<spirv> binaries);
	template<std::convertible_to<spirv>... M>
	spirv link(const M&... modules) {
		std::vector<spirv> binaries; binaries.reserve(sizeof...(modules));
		([&binaries](spirv& module) {
			binaries.emplace_back(module);
			return true;
		}(modules) && ...);

		return link(binaries);
	}

	enum class shader_stage {
		Vertex,
		TesselationControl,
		TesselationEvaluation,
		Geometry,
		Fragment,
		Compute,
		RayGen,
		Intersect,
		AnyHit,
		ClosestHit,
		Miss,
		Callable,
		Task,
		Mesh
	};

	namespace glsl {

#ifdef SLCROSS_ENABLE_READING_GLSL
		spirv parse_from_memory(
			shader_stage stage, 
			const std::string_view content, 
			const std::string_view entry_point = "main",
			client_version version = client_version::Vulkan_1_3
		);
#endif

		std::string generate(spirv_view module, bool target_vulkan = true, bool target_web = false, uint32_t version = 450);
	}

	namespace hlsl {
		std::string generate(spirv_view module, uint32_t shader_model = 30);
	}

	namespace msl {
		std::string generate(spirv_view module, bool target_IOS = false);
	}

#ifdef SLCROSS_ENABLE_SLANG
	namespace slang {
		spirv parse_from_memory(std::string_view content, std::string_view entry_point = "main", std::string_view path = "generated.slang", std::string_view module = "generated");
	}
#endif // SLCROSS_ENABLE_SLANG

#ifdef SLCROSS_ENABLE_WGSL
	namespace wgsl {
		spirv parse_from_memory(const std::string_view content, bool target_vulkan = true, const std::string_view path = "generated.wgsl");
		std::string generate(spirv_view module);
	}
#endif // SLCROSS_ENABLE_WGSL
}