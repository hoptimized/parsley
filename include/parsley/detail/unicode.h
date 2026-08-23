#pragma once

#include <cstdint>
#include <string>

namespace parsley { namespace detail
{
    // TODO: support for UTF16/UTF32
    static void append_utf8(uint32_t cp, std::string& str)
    {
        if (cp <= 0x7F)
        {
            str.push_back(static_cast<char>(cp));
        }
        else if (cp <= 0x7FF)
        {
            str.reserve(str.size() + 2);
            str.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            str.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else if (cp <= 0xFFFF)
        {
            str.reserve(str.size() + 3);
            str.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            str.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            str.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else if (cp <= 0x10FFFF)
        {
            str.reserve(str.size() + 4);
            str.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            str.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            str.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            str.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else
        {
            throw std::runtime_error("Invalid Unicode code point");
        }
    }

    template <typename T>
    static T hex_to_dec(StringView hex)
    {
        T res = 0;

        for (size_t i = 0; i < hex.size(); ++i)
        {
            const char c = hex[i];

            uint8_t value;
            if (c >= '0' && c <= '9')
                value = c - '0';
            else if (c >= 'a' && c <= 'f')
                value = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                value = c - 'A' + 10;
            else
                throw std::runtime_error(std::string{ "Invalid hexadecimal digit:" } + hex[i]);

            res = (res << 4) | static_cast<T>(value);
        }

        return res;
    }

    static void append_unicode_from_hex(StringView hex, size_t digits, std::string& str)
    {        
        if (hex.size() < digits)
            throw std::runtime_error("Incomplete Unicode escape sequence");

        const uint32_t cp = hex_to_dec<uint32_t>(hex.substr(0, digits));

        // Unicode scalar values exclude surrogate code points.
        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
            throw std::runtime_error(std::string{ "Invalid Unicode code point: " } + hex.to_owned());

        append_utf8(cp, str);
    }
}}
