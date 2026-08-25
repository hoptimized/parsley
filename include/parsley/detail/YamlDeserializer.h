#pragma once

#include "parsley/Node.h"
#include "parsley/config/YamlDeserializerConfig.h"
#include "parsley/core/StringView.h"
#include "parsley/detail/escape.h"

#include <string>

namespace parsley { namespace detail
{
    template <class Cursor>
    class YamlDeserializer
    {
    public:
        YamlDeserializer(YamlDeserializerConfig config = {}) {}

        Node read(Cursor& in)
        {
            in_ = &in;
            read_line(); // read the first line
            return parse_block(0);
        }

    private:
        // --- Block Parsing --------------------------------------------------

        /******************************************************************************
         * Parses an unknown block (scalar, sequence, map) at `min_indent` or deeper.
         * 
         * @param min_indent   Minimum indentation required for this block. If the 
         *                     first meaningful text starts at a smaller indentation 
         *                     than `min_indent`, a null-Node will be returned.
         * @param ignore_chars Number of leading characters to ignore on the first line
         *                     of the block. These characters usually belong to an
         *                     enclosing marker on the same physical line, e.g. the 
         *                     "- " in "- - a" or the "a: " in "a: - ". The ignored 
         *                     characters have already been processed by the caller but
         *                     not consumed from the cursor.
         * 
         * @return A Node representing the parsed block.
         ******************************************************************************/
        Node parse_block(size_t min_indent, size_t ignore_chars = 0)
        {
            const bool is_inline_block = ignore_chars > 0;

            if (!is_inline_block)
                skip_structural(); // only meaningful when resuming a fresh line

            if (eof_)
                return {};

            // Calculate the effective indent of this block, which may be inlined
            // (e.g. the "- a" in "  - - a" sits at effective_indent of 4).
            const size_t indent = effective_indent(ignore_chars);

            if (indent < min_indent)
                return {};

            Node node;

            if (try_parse_sequence(indent, ignore_chars, node))
                return node;

            if (try_parse_mapping(indent, ignore_chars, node))
                return node;

            return parse_scalar(indent, ignore_chars);
        }

        // Tries to parse a full sequence: all items, including nested structures.
        // Each item's '-' marker must sit at exactly `required_indent`.
        bool try_parse_sequence(size_t required_indent, size_t ignore_chars, Node& out_seq)
        {
            out_seq = {};

            while (!eof_)
            {
                const size_t indent = effective_indent(ignore_chars);
                const StringView content = effective_content(ignore_chars);

                if (indent != required_indent || !is_sequence_marker(content))
                    break;

                StringView value = content.substr(1); // remove leading '-'
                size_t extra = 1 + strip_leading_whitespace(value);
                size_t item_indent = required_indent + extra;

                if (value.empty())
                {
                    // Nothing more on this physical line; consume it and expect
                    // the item's value on the following line(s).
                    read_line();
                    out_seq.push_back(parse_block(item_indent));
                }
                else
                {
                    // A value follows the marker on this same physical line.
                    // `item_ignore_chars` is how many characters sit before the
                    // real value on the physical line (`content_`).
                    // Example: " - - a" may have indent_=1, item_indent=3 ->
                    // item_ignore_chars=2 (cuts away the parent's "- ").
                    const size_t item_ignore_chars = item_indent - indent_;
                    out_seq.push_back(parse_block(item_indent, item_ignore_chars));
                }

                ignore_chars = 0; // later items start on their own fresh line
            }

            return !out_seq.empty();
        }

        // Tries to parse a mapping with all "key: value" lines.
        // Each key must sit at exactly `required_indent`.
        bool try_parse_mapping(size_t required_indent, size_t ignore_chars, Node& out_map)
        {
            out_map = {};

            // The key could be quoted, with folding and escape sequences.
            // Thus, it cannot be a StringView and must be able to be built.
            std::string key;

            while (!eof_)
            {
                const size_t indent = effective_indent(ignore_chars);
                const StringView content = effective_content(ignore_chars);

                if (indent != required_indent)
                    break;

                key.clear();
                StringView rest;
                if (!is_mapping_kvp(content, key, rest))
                    break;

                read_line();

                // Value must be indented deeper than the key (even if on the same line).
                const size_t value_indent = required_indent + 1;

                if (rest.empty())
                {
                    // Value not on the key line; expect the value on the next line.
                    out_map[key] = parse_block(value_indent);
                }
                else
                {
                    // Value on same line as the key; only a scalar is permitted here.
                    out_map[key] = parse_scalar_value(rest, value_indent);
                }

                ignore_chars = 0;
            }

            return !out_map.empty();
        }

        Node parse_scalar(size_t indent, size_t ignore_chars = 0)
        {
            StringView content = effective_content(ignore_chars);
            read_line();
            return parse_scalar_value(content, indent);
        }

        // --- Sequence Details -----------------------------------------------

        bool is_sequence_marker(StringView content)
        {
            if (content.empty())
                return false;

            if (content.size() == 1 && content.front() == '-')
                return true;

            return content.starts_with("- ");
        }

        // --- Mapping Details ------------------------------------------------

        // Detects "key: rest" at the start of `content`, where key may be 
        // plain or single/double-quoted. Parses the key completely (incl. 
        // substitution of line breaks, escape sequences etc.) and returns the 
        // unprocessed rest of the line.
        bool is_mapping_kvp(StringView content, std::string& out_key, StringView& out_rest)
        {
            if (content.empty())
                return false;

            StringView& s = content;

            if (is_quote(s.front()))
            {
                const char quote = s.front();
                s.remove_prefix(1); // remove the opening quote

                if (!parse_quoted_line(s, quote, out_key))
                {
                    // Unterminated quote while attempting to pass a mapping key.
                    // Implicit mapping keys cannot be multi-line, thus this
                    // cannot be a mapping.
                    return false;
                }
            }
            else
            {
                // Unquoted, try to find a plain key followed by the ':' marker.

                size_t i = 0;
                for (; i < s.size(); ++i)
                {
                    if (s[i] == ':' && (i + 1 == s.size() || s[i + 1] == ' '))
                        break;
                }

                if (i == s.size())
                    return false; // no colon marker found

                out_key = s.substr(0, i).to_owned();
                s.remove_prefix(i);
            }

            // s now starts right where the key token ended.
            // The next characters must be ':' followed by a space or end of line 
            // to count as a mapping marker.
            if (s.empty() || s[0] != ':')
                return false;
            if (s.size() > 1 && s[1] != ' ')
                return false;

            s.remove_prefix(1); // ':'
            strip_leading_whitespace(s);

            out_rest = s;
            return true;
        }

        // --- Scalar Details -------------------------------------------------

        Node parse_scalar_value(StringView first_line, size_t indent)
        {
            if (!first_line.empty() && is_quote(first_line[0]))
                return parse_quoted_scalar_value(first_line);

            return parse_scalar_folded(first_line, indent);
        }

        // Folds in lines whose indent reaches min_indent or deeper:
        // consecutive lines join with a space, a blank line in the
        // run joins with '\n' instead.
        Node parse_scalar_folded(StringView first_line, size_t min_indent)
        {
            std::string text = first_line.to_owned();
            size_t blank_lines = 0;

            while (!eof_)
            {
                if (content_.empty())
                {
                    ++blank_lines;
                    read_line();
                    continue;
                }

                if (indent_ < min_indent)
                    break; // dedent / sibling: fold ends

                StringView content = content_;
                std::string key;
                StringView rest;
                if (is_sequence_marker(content) || is_mapping_kvp(content, key, rest))
                    break; // deeper line that's actually a new construct

                append_fold_separator(text, blank_lines);
                blank_lines = 0;
                text += content.to_owned();
                read_line();
            }

            return Node(std::move(text));
        }

        // Appends the separator that joins two folded lines.
        // Line breaks convert to a space " ". However, a series of n blank
        // lines becomes a series of n line breaks.
        static void append_fold_separator(std::string& text, size_t blank_lines)
        {
            text += blank_lines ? std::string(blank_lines, '\n') : " ";
        }

        // --- Quoted Scalars ---------------------------------------------------

        static bool is_quote(char c)
        {
            return c == '"' || c == '\'';
        }

        Node parse_quoted_scalar_value(StringView first_line)
        {
            StringView& s = first_line; // alias for brevity

            const char quote = first_line.front();
            s.remove_prefix(1);

            std::string text;
            size_t blank_lines = 0;

            for (;;)
            {
                bool line_continuation = false;
                if (parse_quoted_line(s, quote, text, &line_continuation))
                    break; // closing quote found

                if (eof_)
                    break; // unterminated quote at eof

                if (content_.empty())
                {
                    ++blank_lines;
                    read_line();
                    continue;
                }
                
                // Double-quoted scalars may request line continuation with a
                // trailing '\' at the end of a line.
                // If this continuation is requested, we do not fold and instead
                // concatenate text between lines.
                if (!line_continuation)
                    append_fold_separator(text, blank_lines);

                blank_lines = 0;

                s = content_;
                read_line();
            }

            return Node(std::move(text));
        }

        // Parses a line of a quoted scalar.
        // Returns true once the closing quote is consumed.
        // Returns false if the line ended while still inside the quotes.
        // For a double-quoted scalar ending in a lone trailing '\', 
        // *line_continuation is set to true so the caller knows the line
        // break was escaped away, not folded.
        bool parse_quoted_line(
            StringView& line, 
            char quote, 
            std::string& parsed, 
            bool* line_continuation = nullptr)
        {
            StringView& s = line; // alias for brevity

            if (line_continuation != nullptr)
                *line_continuation = false;

            while (!s.empty())
            {
                char c = s.front();

                // ' in single-quoted scalar
                if (quote == '\'' && c == '\'')
                {
                    if (s.size() > 1 && s[1] == '\'')
                    {
                        // '' -> literal '
                        parsed += '\'';
                        s.remove_prefix(2);
                        continue;
                    }

                    s.remove_prefix(1); // closing quote
                    return true;
                }

                // " in double-quoted scalar
                if (quote == '"' && c == '"')
                {
                    s.remove_prefix(1); // closing quote
                    return true;
                }

                // Escape sequences (only meaningful in double-quoted scalars)
                if (quote == '"' && c == '\\')
                {
                    if (s.size() == 1)
                    {
                        // Trailing backslash at end of line: escapes the
                        // line break itself, so no fold separator should
                        // be inserted before the next line's content.
                        s.remove_prefix(1);
                        if (line_continuation != nullptr)
                            *line_continuation = true;
                        return false;
                    }

                    parse_escape_sequence(s, parsed);
                    continue;
                }

                parsed += c;
                s.remove_prefix(1);
            }

            return false;
        }

        // --- Line Helpers ---------------------------------------------------

        static size_t get_indent(StringView line)
        {
            size_t indent = 0;
            while (indent < line.size() && line[indent] == ' ')
                ++indent;
            return indent;
        }

        static size_t strip_leading_whitespace(StringView& sv)
        {
            const size_t indent = get_indent(sv);
            sv.remove_prefix(indent);
            return indent;
        }

        // Calculates the effective content of an inline block with ignored characters.
        // Example: _content="- - a", ignore_chars=2 -> effective_content="- a"
        StringView effective_content(size_t ignore_chars) const
        {
            return content_.substr(ignore_chars);
        }

        // Calculates the effective indent of an inline block with ignored leading characters.
        // Example: _content="  - - a", ignore_chars=2 -> effective_indent=4. This ignores the 
        // first "- " as it belongs to the enclosing block, and our sequence is really "- a",
        // sitting at indent=4.
        size_t effective_indent(size_t ignore_chars) const
        {
            return indent_ + ignore_chars;
        }

        void skip_structural()
        {
            while (!eof_)
            {
                StringView c = content_;

                // blank lines, full-line comments, start marker
                if (c.empty() || c.starts_with("#") || c.starts_with("---"))
                {
                    read_line();
                    continue;
                }

                // end marker
                if (c.starts_with("..."))
                {
                    eof_ = true;
                    return;
                }

                return;
            }
        }

        void read_line()
        {
            if (!in_->get_line(line_))
            {
                eof_ = true;
                return;
            }

            indent_ = get_indent(line_);
            content_ = line_.substr(indent_);
        }

        // --- Members --------------------------------------------------------

        Cursor* in_;
        StringView line_;
        size_t indent_;
        StringView content_;
        bool eof_ = false;
    };
}}
