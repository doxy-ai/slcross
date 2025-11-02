#pragma once

#include <optional>
#include <string>
#include <vector>
#include <span>
#include <map>

#ifndef SLCROSS_IMPLEMENTATION
namespace slang {
	struct IGlobalSession;
	struct ISession;
	struct CompilerOptionEntry;
	struct IComponentType;
}
#endif
#include "slang-com-ptr.h"


namespace slcross {

	struct session {
		struct sessionptr: public ::slang::ISession { using ::slang::ISession::ISession; };

		sessionptr* pointer = nullptr;

		static ::slang::IGlobalSession* maybe_create_global_session();

		static struct session create(std::span<const ::slang::CompilerOptionEntry> options = {});

		Slang::ComPtr<::slang::IComponentType> compose_program(std::string_view entry_point = "main");

		Slang::ComPtr<::slang::IBlob> generate_code(size_t target_index, std::string_view entry_point = "main");

		void free();
	};

	enum shader_stage {
		vertex,
		hull,
		domain,
		geometry,
		mesh,
		amplification,
		fragment,
		raygeneration,
		intersection,
		anyhit,
		closesthit,
		miss,
		callable,
		compute,
		// TODO: Is metal a special snowflake?
	};

	namespace slang {
		session parse_from_memory(std::string_view source, std::string_view path = "generated.slang", std::string_view module = "generated", std::optional<session> existing_session = {});
	}

	namespace spirv {
		std::vector<uint32_t> generate(session session, std::string_view entry_point /* = "main" */);
	}

	namespace glsl {
		std::string generate(session session, std::string_view entry_point /* = "main" */);
	}

	namespace hlsl {
		std::string generate(session session, std::string_view entry_point /* = "main" */);
	}

	namespace msl {
		std::string generate(session session, std::string_view entry_point /* = "main" */);
	}

	namespace wgsl {
		std::string generate(session session, std::string_view entry_point /* = "main" */);
	}

	namespace experimental {
		namespace spirv {
			session parse_from_memory(std::span<const uint32_t> spirv, std::map<shader_stage, std::string_view> entry_points, std::string_view path = "generated.slang", std::string_view module = "generated", std::optional<session> existing_session = {});
		}
	}
}