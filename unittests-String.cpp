#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include "String.hpp"

using namespace Tools;

#define TEST_EQUALS(got, expected) 	{ \
		pid_t pid; \
		if ( (pid = fork()) == 0 ) { \
			auto _expected = expected; \
			auto _got = got; \
			std::cout << "\t" << __FILE__ << "[line #" << __LINE__ << "] " << #expected << " " << #got << "..."; \
			if ( _got == _expected ) { \
				std::cout << "\x1B[32mPASSED.\x1B[0m\n"; \
			} else { \
				std::cout << "\x1B[31mFAILED. Expected \"" << _expected << "\" but got \"" << _got << "\"\x1B[0m" << std::endl; \
			} \
			exit(0);\
		} else { \
			waitpid(pid, nullptr, 0); \
		} \
	}
// #define TEST_EQUALS(expected, got) 	{ \
		auto _expected = expected; \
		auto _got = got; \
		std::cout << "\t" << __FILE__ << "[line#" << __LINE__ << "] " << #expected << " " << #got << "..."; \
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
		String test_string;
		TEST_EQUALS(test_string.GetLength(), 0u);
		TEST_EQUALS(test_string.GetText(), String::EMPTY);
	}

	{	std::cout << "------------------------------String(const char*)-----------------------------" << std::endl;
		String test_string(test_str);
		TEST_EQUALS(test_string.GetLength(), strlen(test_str));
		TEST_EQUALS(String(test_str), test_string);
	}

	{	std::cout << "-----------------------String(const char*, uint offset)-----------------------" << std::endl;
		offset = 1u;
		String test_string = String(test_str, offset);
		TEST_EQUALS(test_string.GetLength(), strlen(test_str + offset));
		TEST_EQUALS(test_string, String(test_str + offset));

		offset = 10;
		test_string = String(test_str, offset);
		TEST_EQUALS(test_string.GetLength(), strlen(test_str + offset));
		TEST_EQUALS(test_string, String(test_str + offset));

		offset = 14u;
		test_string = String(test_str, offset);
		TEST_EQUALS(test_string.GetLength(), strlen(test_str + offset));
		TEST_EQUALS(test_string, String(test_str + offset));

		offset = 15u;
		test_string = String(test_str, offset);
		TEST_EQUALS(0u, test_string.GetLength());
		TEST_EQUALS(test_string, String(""));
	}

	{	std::cout << "-----------------String(const char*, uint offset, uint count)-----------------" << std::endl;
		offset = 1u;
		count = 0u;
		String test_string = String(test_str, offset, count);
		TEST_EQUALS(strlen(test_str + offset), test_string.GetLength());
		TEST_EQUALS(test_string, String("his is a test"));

		count = 1u;
		test_string = String(test_str, offset, count);
		TEST_EQUALS(count, test_string.GetLength());
		TEST_EQUALS(test_string, String("h"));

		count = 5u;
		test_string = String(test_str, offset, count);
		TEST_EQUALS(count, test_string.GetLength());
		TEST_EQUALS(test_string, String("his i"));

		count = 13u;
		test_string = String(test_str, offset, count);
		TEST_EQUALS(count, test_string.GetLength());
		TEST_EQUALS(test_string, String("his is a test"));

		count = 14u;
		test_string = String(test_str, offset, count);
		TEST_EQUALS(count - offset, test_string.GetLength());
		TEST_EQUALS(test_string, String("his is a test"));

		count = 15u;
		test_string = String(test_str, offset, count);
		TEST_EQUALS(13u, test_string.GetLength());
		TEST_EQUALS(test_string, String("his is a test"));
	}

	{ 	std::cout << "--------------------String operator+=(const String& string)-------------------" << std::endl;
		String test_string = "abc";
		test_string += "xyz";
		TEST_EQUALS(test_string, String("abcxyz"));

		test_string = "";
		test_string += "xyz";
		TEST_EQUALS(test_string, String("xyz"));

		test_string = "abc";
		test_string += "";
		TEST_EQUALS(test_string, String("abc"));

		test_string = "";
		test_string += "";
		TEST_EQUALS(test_string, String(""));
	}

	{ 	std::cout << "-------------------String& operator=(const String& string)--------------------" << std::endl;
		String test_string("");
		test_string = "test";
		TEST_EQUALS(test_string, String("test"));
		test_string = "";
		TEST_EQUALS(test_string, String(""));
	}

	{ 	std::cout << "--------------------bool operator==(const String& string)---------------------" << std::endl;
		TEST_EQUALS(String(), "");
		TEST_EQUALS(String("abc"), "abc");
	}

	{	std::cout << "-----------------bool StartsWith(const String& target) const------------------" << std::endl;
		String test_string;
		TEST_TRUE(test_string.StartsWith(""));
		TEST_FALSE(test_string.StartsWith("a"));
		test_string = "";
		TEST_TRUE(test_string.StartsWith(""));
		test_string = "a";
		TEST_TRUE(test_string.StartsWith(""));
		TEST_TRUE(test_string.StartsWith("a"));
		test_string = "aaaaabbbbb";
		TEST_TRUE(test_string.StartsWith("aaaa"));
		TEST_TRUE(test_string.StartsWith("aaaaab"));
		TEST_FALSE(test_string.StartsWith("abaa"));
	}

	{ 	std::cout << "---------------------------bool IsEmpty(void) const---------------------------" << std::endl;
		String test_string;
		TEST_TRUE(test_string.IsEmpty());
		test_string = "asd";
		TEST_FALSE(test_string.IsEmpty());
		test_string = "";
		TEST_TRUE(test_string.IsEmpty());
	}

	{ 	std::cout << "--------------------------uint GetLength(void) const--------------------------" << std::endl;
		String test_string;
		TEST_EQUALS(test_string.GetLength(), 0u);
		test_string = "asdf";
		TEST_EQUALS(test_string.GetLength(), 4u);
	}

	{ 	std::cout << "-----------------------const char* GetText(void) const------------------------" << std::endl;
		String test_string;
		TEST_EQUALS(*test_string.GetText(), 0);
		test_string = "123456";
		TEST_EQUALS(String(test_string.GetText()), "123456");
	}

	// {	std::cout << "----------------------std::vector<String> Split(char c)-----------------------" << std::endl;
	// 	String test_string("");
	// 	std::vector<String> test_strings = test_string.Split(' ');
	// 	TEST_EQUALS(test_strings.size(), 1u);
	// 	TEST_EQUALS(test_strings[0], "");

	// 	test_string = String("abc");
	// 	test_strings = test_string.Split(' ');
	// 	TEST_EQUALS(test_strings.size(), 1u);
	// 	TEST_EQUALS(test_strings[0], "abc");

	// 	test_string = String("abc ");
	// 	test_strings = test_string.Split(' ');
	// 	TEST_EQUALS(test_strings.size(), 2u);
	// 	TEST_EQUALS(test_strings[0], "abc");
	// 	TEST_EQUALS(test_strings[1], "");

	// 	test_string = String("abc def");
	// 	test_strings = test_string.Split(' ');
	// 	TEST_EQUALS(test_strings.size(), 2u);
	// 	TEST_EQUALS(test_strings[0], "abc");
	// 	TEST_EQUALS(test_strings[1], "def");

	// 	test_string = String("This is a test of the ");
	// 	test_strings = test_string.Split(' ');
	// 	TEST_EQUALS(test_strings.size(), 7u);
	// 	TEST_EQUALS(test_strings[0], "This");
	// 	TEST_EQUALS(test_strings[1], "is");
	// 	TEST_EQUALS(test_strings[2], "a");
	// 	TEST_EQUALS(test_strings[3], "test");
	// 	TEST_EQUALS(test_strings[4], "of");
	// 	TEST_EQUALS(test_strings[5], "the");
	// 	TEST_EQUALS(test_strings[6], "");
	// }
}
