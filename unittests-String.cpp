#include <string.h>
#include <iostream>
#include "String.hpp"

using namespace Tools;

#define TEST_EQUALS(expected, got) 	{ \
								auto _expected = expected; \
								auto _got = got; \
								std::cout << "\t" << __FILE__ << "[" << __LINE__ << "] " << #expected << " " << #got << "..."; \
								if ( _expected == _got ) { \
									std::cout << "\x1B[32mPASSED.\x1B[0m\n"; \
								} else { \
									std::cout << "\x1B[31mFAILED. Expected \"" << _expected << "\" but got \"" << _got << "\"\x1B[0m" << std::endl; \
								} \
							}
#define TEST_TRUE(test) 	{ \
								auto _test = test; \
								std::cout << "\t" << __FILE__ << "[" << __LINE__ << "]" << "..."; \
								std::cout << "\x1B[32m" << (_test? "PASSED": "FAILED") << ".\x1B[0m" << std::endl; \
							}
#define TEST_FALSE(test) 	{ \
								auto _test = test; \
								std::cout << "\t" << __FILE__ << "[" << __LINE__ << "]" << "..."; \
								std::cout << "\x1B[32m" << (!_test? "PASSED": "FAILED") << ".\x1B[0m" << std::endl; \
							}

typedef unsigned int uint;

//============================================================================================================================

void unit_tests(void) {
	uint offset = 0u;
	uint count = 0u;
	static const char *test_str = "This is a test";

	{	std::cout << "-----------------------------------String()-----------------------------------" << std::endl;
		String string;
		TEST_EQUALS(string.GetLength(), 0u);
		TEST_EQUALS(String::EMPTY, string.GetText());
	}

	{	std::cout << "------------------------------String(const char*)-----------------------------" << std::endl;
		String string(test_str);
		TEST_EQUALS(string.GetLength(), strlen(test_str));
		TEST_EQUALS(String(test_str), string);
	}

	{	std::cout << "-----------------------String(const char*, uint offset)-----------------------" << std::endl;
		offset = 1u;
		String string = String(test_str, offset);
		TEST_EQUALS(string.GetLength(), strlen(test_str + offset));
		TEST_EQUALS(String(test_str + offset), string);

		offset = 10;
		string = String(test_str, offset);
		TEST_EQUALS(string.GetLength(), strlen(test_str + offset));
		TEST_EQUALS(String(test_str + offset), string);

		offset = 14u;
		string = String(test_str, offset);
		TEST_EQUALS(string.GetLength(), strlen(test_str + offset));
		TEST_EQUALS(string, test_str + offset);

		offset = 15u;
		string = String(test_str, offset);
		TEST_EQUALS(0u, string.GetLength());
		TEST_EQUALS(String(""), string);
	}

	{	std::cout << "-----------------String(const char*, uint offset, uint count)-----------------" << std::endl;
		offset = 1u;
		count = 0u;
		String string = String(test_str, offset, count);
		TEST_EQUALS(strlen(test_str + offset), string.GetLength());
		TEST_EQUALS(String("his is a test"), string);

		count = 1u;
		string = String(test_str, offset, count);
		TEST_EQUALS(count, string.GetLength());
		TEST_EQUALS(String("h"), string);

		count = 5u;
		string = String(test_str, offset, count);
		TEST_EQUALS(count, string.GetLength());
		TEST_EQUALS(String("his i"), string);

		count = 13u;
		string = String(test_str, offset, count);
		TEST_EQUALS(count, string.GetLength());
		TEST_EQUALS(String("his is a test"), string);

		count = 14u;
		string = String(test_str, offset, count);
		TEST_EQUALS(count - offset, string.GetLength());
		TEST_EQUALS(String("his is a test"), string);

		count = 15u;
		string = String(test_str, offset, count);
		TEST_EQUALS(13u, string.GetLength());
		TEST_EQUALS(String("his is a test"), string);
	}

	{ 	std::cout << "--------------------String operator+=(const String& string)-------------------" << std::endl;
		String string = "abc";
		string += "xyz";
		TEST_EQUALS(String("abcxyz"), string);

		string = "";
		string += "xyz";
		TEST_EQUALS(String("xyz"), string);

		string = "abc";
		string += "";
		TEST_EQUALS(String("abc"), string);

		string = "";
		string += "";
		TEST_EQUALS(String(""), string);
	}

	{ 	std::cout << "-------------------String& operator=(const String& string)--------------------" << std::endl;
		String string("");
		string = "test";
		TEST_EQUALS(String("test"), string);
		string = "";
		TEST_EQUALS(String(""), string);
	}

	{ 	std::cout << "--------------------bool operator==(const String& string)---------------------" << std::endl;
		TEST_EQUALS(String(), "");
		TEST_EQUALS(String("abc"), "abc");
	}

	{	std::cout << "-----------------bool StartsWith(const String& target) const------------------" << std::endl;
		String string;
		TEST_TRUE(string.StartsWith(""));
		TEST_FALSE(string.StartsWith("a"));
		string = "";
		TEST_TRUE(string.StartsWith(""));
		string = "a";
		TEST_TRUE(string.StartsWith(""));
		TEST_TRUE(string.StartsWith("a"));
		string = "aaaaabbbbb";
		TEST_TRUE(string.StartsWith("aaaa"));
		TEST_TRUE(string.StartsWith("aaaaab"));
		TEST_FALSE(string.StartsWith("abaa"));
	}

	{ 	std::cout << "---------------------------bool IsEmpty(void) const---------------------------" << std::endl;
		String string;
		TEST_TRUE(string.IsEmpty());
		string = "asd";
		TEST_FALSE(string.IsEmpty());
		string = "";
		TEST_TRUE(string.IsEmpty());
	}

	{ 	std::cout << "--------------------------uint GetLength(void) const--------------------------" << std::endl;
		String string;
		TEST_EQUALS(string.GetLength(), 0u);
		string = "asdf";
		TEST_EQUALS(string.GetLength(), 4u);
	}

	{ 	std::cout << "-----------------------const char* GetText(void) const------------------------" << std::endl;
		String string;
		TEST_EQUALS(string.GetText(), "");
		string = "123456";
		TEST_EQUALS(String(string.GetText()), "123456");
	}

	{	std::cout << "----------------------std::vector<String> Split(char c)-----------------------" << std::endl;
		String string("");
		std::vector<String> strings = string.Split(' ');
		TEST_EQUALS(strings.size(), 1u);
		TEST_EQUALS(strings[0], "");

		string = String("abc");
		strings = string.Split(' ');
		TEST_EQUALS(strings.size(), 1u);
		TEST_EQUALS(strings[0], "abc");

		string = String("abc ");
		strings = string.Split(' ');
		TEST_EQUALS(strings.size(), 2u);
		TEST_EQUALS(strings[0], "abc");
		TEST_EQUALS(strings[1], "");

		string = String("abc def");
		strings = string.Split(' ');
		TEST_EQUALS(strings.size(), 2u);
		TEST_EQUALS(strings[0], "abc");
		TEST_EQUALS(strings[1], "def");

		string = String("This is a test of the ");
		strings = string.Split(' ');
		TEST_EQUALS(strings.size(), 7u);
		TEST_EQUALS(strings[0], "This");
		TEST_EQUALS(strings[1], "is");
		TEST_EQUALS(strings[2], "a");
		TEST_EQUALS(strings[3], "test");
		TEST_EQUALS(strings[4], "of");
		TEST_EQUALS(strings[5], "the");
		TEST_EQUALS(strings[6], "");
	}
}
