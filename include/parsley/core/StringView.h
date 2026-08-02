#pragma once

#include <cstring>
#include <ostream>
#include <stdexcept>
#include <string>

namespace parsley
{
    class StringView
    {
        using CharT = char;

    public:
        static constexpr size_t npos = size_t(-1);

        StringView() : ptr_(nullptr), len_(0) {}
        StringView(const CharT* str) : ptr_(str), len_(std::strlen(str)) {}
        StringView(const CharT* str, size_t count) : ptr_(str), len_(std::min(count, std::strlen(str))) {}
        StringView(const std::string& str) : ptr_(str.data()), len_(str.size()) {}

        StringView& operator=(const StringView& other)
        {
            ptr_ = other.ptr_;
            len_ = other.len_;
            return *this;
        }

        std::string to_owned() const
        {
            return std::string{ ptr_, len_ };
        }

        const char* data() const { return ptr_; }
        size_t size() const { return len_; }
        bool empty() const { return len_ == 0; }

        bool starts_with(StringView prefix) const
        {
            const size_t n = prefix.size();

            if (size() < n)
                return false;

            return substr(0, n) == prefix;
        }

        bool ends_with(StringView prefix) const
        {
            const size_t n = prefix.size();

            if (size() < n)
                return false;

            return substr(len_ - n, n) == prefix;
        }

        const CharT& operator[](size_t pos) const 
        {
            if (pos > size())
                throw std::out_of_range("Cannot access StringView beyond its end");

            return *(ptr_ + pos);
        }

        const CharT& front() const
        {
            if (empty())
                throw std::out_of_range("Cannot access front of empty StringView");
            
            return (*this)[0]; 
        }

        const CharT& back() const
        {
            if (empty())
                throw std::out_of_range("Cannot access front of empty StringView");
            
            return (*this)[size() - 1]; 
        }

        StringView substr(size_t pos = 0, size_t count = npos) const
        {
            if (pos > size())
                throw std::out_of_range("pos cannot be greater than size()");

            const size_t rlen = std::min(count, size() - pos);
            return StringView{ ptr_ + pos, rlen };
        }

        void remove_prefix(size_t n)
        {
            if (n > size())
                throw std::out_of_range("Cannot remove prefix that exceeds len_");

            ptr_ += n;
            len_ -= n;
        }

        void remove_suffix(size_t n)
        {
            if (n > size())
                throw std::out_of_range("Cannot remove suffix that exceeds len_");

            len_ -= n;
        }

        bool operator==(const StringView& other) const
        {
            if (len_ != other.len_)
                return false;
            
            return std::memcmp(ptr_, other.data(), len_) == 0;
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

    inline std::ostream& operator<< (std::ostream& stream, StringView sv)
    {
        return stream.write(sv.data(), static_cast<std::streamsize>(sv.size()));
    }
}
