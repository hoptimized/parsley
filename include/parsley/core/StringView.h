#pragma once

#include <cstring>
#include <string>

namespace parsley
{
    class StringView
    {
        using CharT = char;

    public:
        StringView() : ptr_(nullptr), len_(0) {}
        StringView(const CharT* str) : ptr_(str), len_(std::strlen(str)) {}
        StringView(const std::string& str) : ptr_(str.data()), len_(str.size()) {}        

        operator std::string() const
        {
            return std::string{ ptr_, len_ };
        }

        const char* data() const { return ptr_; }
        size_t size() const { return len_; }
        bool empty() const { return len_ == 0; }

        bool operator==(const std::string& str) const
        {
            if (len_ != str.size())
                return false;
            
            return std::memcmp(ptr_, str.data(), len_) == 0;
        }

    private:
        const CharT* ptr_;
        size_t len_;
    };

    inline bool operator==(const std::string& lhs, const StringView& rhs)
    {
        if (lhs.size() != rhs.size())
            return false;

        return std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
    }
}
