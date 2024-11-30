#pragma once

#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace slcross {
	struct error : std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	namespace detail {
		inline std::string process_pragma_once(std::string data, const std::filesystem::path& path) {
			if(auto once = data.find("#pragma once"); once != std::string::npos) {
				std::string guard = path.string();
				std::transform(guard.begin(), guard.end(), guard.begin(), [](char c) -> char {
					if(!std::isalnum(c)) return '_';
					return std::toupper(c);
				});
				guard = "__" + guard + "_GUARD__";
				data.replace(once, 12, "#ifndef " + guard + "\n#define " + guard + "\n");
				data += "#endif //" + guard + "\n";
			}
			return data;
		}

		inline std::string remove_whitespace(std::string result) {
			result.erase(std::remove_if(result.begin(), result.end(), ::isspace), result.end());
			return result;
		}
	}

	inline std::string read_entire_file(std::filesystem::path path, bool support_pragma_once) {
		std::ifstream fin(path);
		if(!fin) throw error("Failed to open file `" + path.string() + "`... does it exist?");

		fin.seekg(0, std::ios::end);
		size_t size = fin.tellg();
		fin.seekg(0, std::ios::beg);

		std::string data(size + 1, '\n');
		fin.read(data.data(), size);
		if(support_pragma_once)
			return detail::process_pragma_once(data, path);
		return data;
	}
}