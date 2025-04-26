#include <iostream>
#include <vector>
#include <algorithm>
#include "thirdparty/mio.hpp"

namespace slcross {
	inline std::vector<std::byte> read_entire_file(const std::string_view path) {
		if(path == ".") {
			std::cin >> std::noskipws;
			std::istream_iterator<char> it(std::cin);
			std::istream_iterator<char> end;
			std::vector<std::byte> out;
			auto inserter = std::back_inserter(out);
			std::copy(it, end, (std::back_insert_iterator<std::vector<char>>&)inserter);
			return out;
		}

		mio::mmap_source mmap(path, 0, mio::map_entire_file);
		return std::vector<std::byte>((std::byte*)mmap.begin(), (std::byte*)mmap.end());
	}

	template<typename T>
	inline std::vector<T> read_entire_file_as(const std::string_view path) {
		if(path == ".") {
			std::cin >> std::noskipws;
			std::istream_iterator<char> it(std::cin);
			std::istream_iterator<char> end;
			std::vector<char> tmp(it, end);
			return std::vector<T>{(T*)tmp.data(), (T*)(tmp.data() + tmp.size())};
		}

		mio::mmap_source mmap(path, 0, mio::map_entire_file);
		return std::vector<T>((T*)mmap.begin(), (T*)mmap.end());
	}

	inline std::string read_entire_file_as_string(const std::string_view path) {
		if(path == ".") {
			std::cin >> std::noskipws;
			std::istream_iterator<char> it(std::cin);
			std::istream_iterator<char> end;
			return std::string(it, end);
		}

		mio::mmap_source mmap(path, 0, mio::map_entire_file);
		return std::string(mmap.begin(), mmap.end());
	}
}