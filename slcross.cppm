module;

#include "slcross.hpp"

export module slcross;

export namespace slcross {
	using slcross::session;
	using slcross::shader_stage;

	namespace slang {
		using slcross::slang::parse_from_memory;
	}

	namespace spirv {
		using slcross::spirv::generate;
	}

	namespace glsl {
		using slcross::glsl::generate;
	}

	namespace hlsl {
		using slcross::hlsl::generate;
	}

	namespace msl {
		using slcross::msl::generate;
	}

	namespace wgsl {
		using slcross::wgsl::generate;
	}

	namespace experimental {
		namespace spirv {
			using slcross::experimental::spirv::parse_from_memory; 
		}
	}
}