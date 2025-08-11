#include <string.h>
#include "String.hpp"

namespace Tools {
/** String::String(const char *str, uint start, uint bytes)
 *
 * 	Purpose: create a String from 'str' starting at 'start' and filling 'bytes' count
 *
 * 	Notes: Yes, this is inefficient. Even the fact that C++ will convert types is a
 * 		problem, but this is "throwaway" code.
 */
	String::String(const char *str, uint start, uint bytes) {
		if ( str != nullptr ) {
			length = strlen(str);

			if ( length == 0  ||  start >= length ) {
				length = 0;
				text = const_cast<char*>(EMPTY);

			} else {

			//--- Starting at the front and all bytes: full string.
				if ( start == 0  &&  bytes == 0 ) {
//NOOP				len = len;

			//--- Starting at the front and copying 'bytes' as long as 'bytes' is less than 'length'
				} else if ( start == 0  &&  bytes > 0  &&  bytes < length ) {
					length = bytes;

			//--- Start at front but 'bytes' is greater than length, copy 'length' bytes.
				} else if ( start == 0  &&  bytes > 0  &&  bytes >= length ) {
//NOOP				len = len;

			//--- From 'start' (>0) and unlimited count, copy the difference of 'length' and 'start'
				} else if ( start > 0  &&  bytes == 0 ) {
					length -= start;

			//--- As long as 'bytes' + 'start' is less than 'length', copy 'bytes' count
				} else if ( start > 0  &&  bytes > 0  &&  bytes + start < length ) {
					length = bytes;

			//--- If 'bytes' + 'start' are greater than 'length', copy 'length' - 'start' bytes
				} else if ( start > 0  &&  bytes > 0  &&  bytes + start >= length ) {
					length -= start;
				}

			//--- Do the copy.
				if ( length > 0 ) {
					text = strncpy(new char[length + 1], str + start, length + 1);
					text[length] = 0;
				}
			}

		} else {
			text = const_cast<char*>(EMPTY);
		}
	}

/** String::String(long long value)
 *
 * 	PURPOSE: Convert an integer into a string.
 *
 * 	__BASIC_ALGORITHM__
 * 		* Use snprintf() to handle the 'long long'
 */
	String::String(long long value, uint radix) {
		static const char *Letters = "0123456789ABCDEF";
		char buffer[200], *tmps = buffer + (sizeof(buffer) - 2);
		memset(buffer, ' ', sizeof(buffer));
		bool is_neg = false;
		if ( value < 0 ) {
			is_neg = true;
			value = -value;
		}
		*--tmps = 0;
		if ( value != 0 ) {
			while ( value > 0 ) {
				*--tmps = Letters[value % radix];
				value /= radix;
			}
		} else {
			*--tmps = '0';
		}
		if ( is_neg ) {
			*--tmps = '-';
		}
		length = strlen(tmps);
		text = new char[length + 1];
		strncpy(text, tmps, length);
		text[length] = 0;
	}

/** String::String(const String& string): length(string.length)
 *
 */
	String::String(const String& string): length(string.length) {
		text = (!string.IsEmpty()? strdup(string.text): const_cast<char*>(EMPTY));
	}

/** String::~String(void)
 *
 */
	String::~String(void) {
		if ( *text != 0 ) {
			delete [] text;
			text = EMPTY; // <-- reset for double-free errors
			length = 0;
		}
	}

/** std::vector<String> String::Split(char c)
 *
 */
	std::vector<String> String::Split(char c) {
		std::vector<String> results;
		uint head = 0, tail = 0;
		while ( text[head] != 0  &&  head != 0  &&  head < length ) {
			if ( text[head] == c ) {
				results.push_back(String(text + tail, head - tail));
				tail = head;
			}
			head++;
		}
		if ( tail < head ) {
			results.push_back(String(text + tail, head - tail));
		}
		return results;
	}

/** String& String::operator=(const String& string)
 *
 */
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

/** bool String::operator==(const String& string)
 *
 */
	bool String::operator==(const String& string) {
		return (strcmp(text, string.text) == 0);
	}

/** String String::operator+=(const String& string)
 *
 */
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

/** String String::operator+(const String& string)
 *
 */
	String String::operator+(const String& string) {
		size_t len = length + string.length;
		char tmps[len + 1];
		snprintf(tmps, len + 1, "%s%s", text, string.text);
		return String(tmps);
	}

/** bool String::StartsWith(const String& target) const
 *
 */
	bool String::StartsWith(const String& target) const {
		if ( length >= target.length ) {
			const char *str = text;
			const char *sub = target.text;
			while ( *sub != 0  &&  *str == *sub ) {
				str++, sub++;
			}
			return (*sub == 0);

		} else {
			return false;
		}
	}
};
