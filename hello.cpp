#include "slcross.hpp"

#include <spirv-tools/libspirv.hpp>
#include <iostream>

int main() {
	std::string test = R"_slang(
struct VSInput {
    float3 position : POSITION;    // object-space position
};

struct VSOutput {
    float4 position : SV_Position; // clip-space position
};

[shader("vertex")]
VSOutput vertex(VSInput in) {
    VSOutput out;
    out.position = float4(in.position, 1.0);
    return out;
}

[shader("fragment")]
float4 fragment(VSOutput in) : SV_Target {
    return float4(1.0, 0.5, 0.0, 1.0);
}
	)_slang";

	auto session = slcross::slang::parse_from_memory(test);
	auto spirv = slcross::spirv::generate(session, "vertex");

	spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_3);
	tools.SetMessageConsumer([](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) {
		std::cout << position.line << ":" << position.column << " error: " << message << "\n";
	});

	bool valid = tools.Validate(spirv.data(), spirv.size());

	auto session2 = slcross::experimental::spirv::parse_from_memory(spirv, {{slcross::shader_stage::vertex, "main"}});
	auto wgsl = slcross::wgsl::generate(session2, "main");
	std::cout << wgsl << std::endl;
}