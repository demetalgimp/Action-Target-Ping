#ifndef STRING_HPP
#define STRING_HPP

#include <stdlib.h>
#include <vector>
#include <iostream>

namespace Tools {
    class String {
        char *text = nullptr;
        size_t length = 0;
        public:
            static constexpr char *EMPTY = const_cast<char*>("");

        public:
            String(const char *str = nullptr, uint start = 0, uint bytes = 0);
            String(long long value, uint radix=10);
            String(const String& string);
            virtual ~String(void);

        public:
            std::vector<String> Split(char c);

        public:
            String& operator=(const String& string);
            friend std::ostream& operator<<(std::ostream& stream, const String& string) {
                return (stream << string.GetText());
            }
            friend String operator+(const char *str, const String& string) {
                size_t len = strlen(str) + string.length;
                char text[len + 1];
                strcpy(text, str);
                strcat(text, string.text);
                text[len] = 0;
                return String(text);
            }
            bool operator==(const String& string);
            String operator+=(const String& string);
            String operator+(const String& string);

        public:
            bool StartsWith(const String& target) const;
            bool IsEmpty(void) const            { return *text == 0; };
            size_t GetLength(void) const        { return length; }
            const char* GetText(void) const     { return text; }
    };
};

#endif
