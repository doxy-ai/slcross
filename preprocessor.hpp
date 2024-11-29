#pragma once

#include "common.hpp"

#ifdef SLCROSS_IMPLEMENTATION
	#define TCPP_IMPLEMENTATION
	#include "thirdparty/tcppLibrary.hpp"
#endif

#include <set>
#include <unordered_map>
#include <filesystem>


namespace slcross {
	namespace detail {
		inline std::string consolidate_whitespace(const std::string& in) {
			std::string out; out.reserve(in.size());
			for (size_t i = 0, count = 0, size = in.size(); i < size; ++i)
				if (std::isspace(in[i]))
					++count;
				else {
					if (count > 0) { // Replace multiple whitespace with a single space
						out += ' ';
						count = 0;
					}
					out += in[i];
				}

			// Handle trailing/leading whitespace characters
			while (out.length() >= 2 && out.back() == ' ') // NOTE: All whitespace has been converted to spaces at this point
				out.pop_back();
			if(out[0] == ' ') return out.substr(1);
			else return out;
		}
	}

	struct preprocessor {
		struct config {
			bool remove_comments = false;
			bool remove_whitespace = false;
			bool support_pragma_once = true;
			// bool include_only = false; // Don't process any #defines // TODO: implement support for only processing includes
			const char* path = nullptr;
		};

		std::unordered_map<std::filesystem::path, std::string> file_cache;

		std::set<std::filesystem::path> search_paths;
		std::set<std::string> defines = {};

		std::string defines_string() {
			std::string out = "\n";
			for(auto& def: defines)
				out += def + "\n";
			return out;
		}

		std::string process_from_memory(std::string_view data, const config& config);

		std::string process_from_memory_and_cache(const std::string& data_, const std::filesystem::path& path, config config) {
			std::string data;
			if(config.support_pragma_once) data = detail::process_pragma_once(data_, path);
			else data = data_;

			auto str = path.string(); config.path = str.c_str();
			data = process_from_memory(data, config);
			file_cache[path] = data;
			return data;
		}

		std::string process(const std::filesystem::path& path, const config& config) {
			if(file_cache.find(path) != file_cache.end())
				return file_cache[path];

			auto data = read_entire_file(path, config.support_pragma_once);
			return process_from_memory_and_cache(data, path, config);
		}

		preprocessor& add_define(std::string_view name, std::string_view value) {
			defines.insert("#define " + std::string(name) + " " + std::string(value));
			return *this;
		}

		// Note: This function is very expensive!
		preprocessor& remove_define(std::string_view name) {
			auto iter = std::find_if(defines.begin(), defines.end(), [name](const std::string& define) {
				return std::search(define.begin(), define.end(), name.begin(), name.end()) != define.end();
			});

			if(iter != defines.end())
				defines.erase(iter);
			return *this;
		}
	};

#ifdef SLCROSS_IMPLEMENTATION
	std::string preprocessor::process_from_memory(std::string_view data_, const config& config) {
		auto data = std::make_unique<tcpp::StringInputStream>(defines_string() + std::string(data_) + "\n");
		tcpp::Lexer lexer(std::move(data));
		tcpp::Preprocessor preprocessor(lexer, {[&config](const tcpp::TErrorInfo& info){
			auto errorString = tcpp::ErrorTypeToString(info.mType);
			if(errorString.empty()) errorString = "Unknown error";
			if(config.path) errorString = errorString + "in" + config.path;
			errorString += " on line: " + std::to_string(info.mLine);
			// TODO: Include that line of the input!
			throw error{errorString};
		}, [&](const std::string& path_, bool isSystem) -> tcpp::TInputStreamUniquePtr{
			std::filesystem::path path = path_;
			if(file_cache.find(path) != file_cache.end())
				return std::make_unique<tcpp::StringInputStream>(file_cache[path]);

			#define SHADER_PREPROCESSOR_NON_SYSTEM_PATHS {\
				if(config.path) {\
					auto relativeToConfig = std::filesystem::absolute(std::filesystem::path(config.path).parent_path() / path);\
					if(std::filesystem::exists(relativeToConfig))\
						return std::make_unique<tcpp::StringInputStream>(read_entire_file(relativeToConfig, config.support_pragma_once));\
				}\
				auto absolute = std::filesystem::absolute(path);\
				if(std::filesystem::exists(absolute))\
					return std::make_unique<tcpp::StringInputStream>(read_entire_file(absolute, config.support_pragma_once));\
			}
			if(!isSystem) SHADER_PREPROCESSOR_NON_SYSTEM_PATHS

			for(auto system: search_paths) {
				system = system / path;
				if(std::filesystem::exists(system))
					return std::make_unique<tcpp::StringInputStream>(read_entire_file(system, config.support_pragma_once));
			}

			if(isSystem) SHADER_PREPROCESSOR_NON_SYSTEM_PATHS
			#undef SHADER_PREPROCESSOR_NON_SYSTEM_PATHS

			throw error{"Included file `" + path.string() + "` could not be found!"};
		}, config.remove_comments});

		if(config.remove_whitespace) return detail::consolidate_whitespace(preprocessor.Process());
		return preprocessor.Process();
	}
#endif
}