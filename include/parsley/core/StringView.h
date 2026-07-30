#pragma once

#include <cstring>
#include <string>

namespace parsley
{
    class StringView
    {
        using CharT = char;

    public:
        StringView(const CharT* str) : ptr_(str), len_(std::strlen(str)) {}
        StringView(const std::string& str) : ptr_(str.data()), len_(str.size()) {}        

        operator std::string() const
        {
            return std::string{ ptr_, len_ };
        }

        bool operator==(const std::string& str)
        {
            if (len_ != str.size())
                return false;

            return std::memcmp(ptr_, str.data(), std::min(len_, str.size())) == 0;
        }

    private:
        const CharT* ptr_;
        size_t len_;
    };

    inline bool operator==(const std::string& lhs, const StringView& rhs) { return rhs == lhs; }
}
