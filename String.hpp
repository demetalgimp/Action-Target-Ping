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
            String(long long value);
            String(const String& string);
            virtual ~String(void);

        public:
            std::vector<String> Split(char c);

        public:
            String& operator=(const String& string);
            friend std::ostream& operator<<(std::ostream& stream, const String& string) {
                return (stream << string.GetText());
            }
            bool operator==(const String& string);
            String operator+=(const String& string);

        public:
            bool StartsWith(const String& target) const;
            bool IsEmpty(void) const;
            size_t GetLength(void) const;
            const char* GetText(void) const;
    };
};