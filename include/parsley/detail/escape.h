#pragma once

#include "parsley/core/StringView.h"
#include "parsley/detail/unicode.h"

#include <string>

namespace parsley { namespace detail
{
    static void parse_escape_sequence(StringView& sequence, std::string& out_str)
    {
        sequence.remove_prefix(1); // strip the leading backslash

        const char sequence_type = sequence.front();
        sequence.remove_prefix(1);

        switch (sequence_type)
        {
            // Single-char sequences
            case '0': out_str += '\0'; break; // null
            case 'a': out_str += '\a'; break; // bell
            case 'b': out_str += '\b'; break; // backspace
            case 't': out_str += '\t'; break; // horizontal tab
            case 'n': out_str += '\n'; break; // line feed
            case 'v': out_str += '\v'; break; // vertical tab
            case 'f': out_str += '\f'; break; // form feed
            case 'r': out_str += '\r'; break; // carriage return
            case 'e': out_str += '\e'; break; // escape
            case '"': out_str += '"'; break; // double quote
            case '/': out_str += '/'; break; // slash
            case '\\': out_str += '\\'; break; // backslash
            case 'N': out_str += "\u0085"; break; // next line
            case '_': out_str += "\u00A0"; break; // non-breaking space
            case 'L': out_str += "\u2028"; break; // line separator
            case 'P': out_str += "\u2029"; break; // paragraph separator

            // ASCII/Unicode 8bit sequence
            case 'x':
                append_unicode_from_hex(sequence, 2, out_str);
                sequence.remove_prefix(2);
                break;

            // Unicode 16bit sequence
            case 'u':
                append_unicode_from_hex(sequence, 4, out_str);
                sequence.remove_prefix(4);
                break;

            // Unicode 32bit sequence
            case 'U':
                append_unicode_from_hex(sequence, 8, out_str);
                sequence.remove_prefix(8);
                break;

            // Fallthrough
            default: 
                throw std::runtime_error(std::string{ "Unknown escape sequence: \\" } + sequence_type);
        }
    }
}}
