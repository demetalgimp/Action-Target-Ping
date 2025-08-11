#include <string.h>
#include "String.hpp"

namespace Tools {
	String::String(const char *str, uint start, uint bytes) {
		if ( str != nullptr ) {
			length = strlen(str);

			if ( length == 0  ||  start >= length ) {
				length = 0;
				text = const_cast<char*>(EMPTY);

			} else {
				if ( start == 0  &&  bytes == 0 ) {
//NOOP				len = len;

				} else if ( start == 0  &&  bytes > 0  &&  bytes < length ) {
					length = bytes;

				} else if ( start == 0  &&  bytes > 0  &&  bytes >= length ) {
//NOOP				len = len;

				} else if ( start > 0  &&  bytes == 0 ) {
					length -= start;

				} else if ( start > 0  &&  bytes > 0  &&  bytes + start < length ) {
					length = bytes;

				} else if ( start > 0  &&  bytes > 0  &&  bytes + start >= length ) {
					length -= start;
				}

				if ( length > 0 ) {
					text = strncpy(new char[length + 1], str + start, length + 1);
					text[length] = 0;
				}
			}

		} else {
			text = const_cast<char*>(EMPTY);
		}
	}
	String::String(long long value) {
		char tmps[25];
		snprintf(tmps, sizeof(tmps), "%lld", value);
		length = strlen(tmps);
		text = new char[length + 1];
		strncpy(text, tmps, length);
	}
	String::String(const String& string): length(string.length) {
		text = (!string.IsEmpty()? strdup(string.text): const_cast<char*>(EMPTY));
	}
	String::~String(void) {
		if ( *text != 0 ) {
			delete [] text;
			text = EMPTY; // <-- reset for double-free errors
			length = 0;
		}
	}
	// std::vector<String> String::Split(char c) {
	// 	std::vector<String> results;
	// 	uint head = 0, tail = 0;
	// 	while ( text[head] != 0  &&  head < length ) {
	// 		if ( text[head] == c ) {
	// 			results.push_back(String(text + tail, head - tail));
	// 			tail = head;
	// 		}
	// 		head++;
	// 	}
	// 	if ( tail < head ) {
	// 		results.push_back(String(text + tail, head - tail));
	// 	}
	// 	return results;
	// }
	String& String::operator=(const String& string) {
		if ( text != EMPTY ) {
			delete [] text;
			text = EMPTY;
			length = 0;
		}
		if ( string.IsEmpty() ) { // <-- If the param 'string' == EMPTY, clear *this
			length = 0;
			text = EMPTY;

		} else {
			text = strdup(string.text);
			length = string.length;
		}

		return *this;
	}
	bool String::operator==(const String& string) {
		return (strcmp(text, string.text) == 0);
	}
	String String::operator+=(const String& string) {
		if ( !string.IsEmpty() ) {
			length += string.length;
			char *tmps = new char[length + 1];
			snprintf(tmps, length + 1, "%s%s", text, string.text);
			if ( text != EMPTY ) {
				delete [] text;
			}
			text = tmps;
		}
		return *this;
	}
	bool String::StartsWith(const String& target) const {
		const char *str = text;
		const char *sub = target.text;
		while ( *sub != 0  &&  *str == *sub ) {
			str++, sub++;
		}
		return (*sub == 0);
	}
	bool String::IsEmpty(void) const {
		return (*text == 0);
	}
	size_t String::GetLength(void) const {
		return length;
	}
	const char* String::GetText(void) const {
		return text;
	}
};
