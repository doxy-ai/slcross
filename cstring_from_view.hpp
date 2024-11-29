#include <string>
	
// Creates a c-string from a string view
// (if the string view doesn't point to a valid cstring a temporary one that
//		is only valid until the next time this function is called is created)
template<size_t uniqueID = 0>
inline const char* cstring_from_view(const std::string_view view) {
	static std::string tmp;
	if(view.data()[view.size()] == '\0') return view.data();
	tmp = view;
	return tmp.c_str();
}