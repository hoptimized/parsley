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

            // TODO:
            // What we should actually do here:
            //  - check if it's a sequence (sequence marker) -> parse sequence
            //  - if not a sequence -> parse scalar
            //  - if the scalar was single-line (maybe just return an int indicating lines parsed):
            //      - check if the next char on the same line is ": " or ":\n"
            //      - if yes, parse a mapping -> re-use the already parsed scalar as the key
            // This should avoid the awkward double-parsing and the fact that try_parse_mapping is "fat".
            // Ideally, we wouldn't need any "try_***" methods at all.

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

                // Value must be indented deeper than the key (even if on the same line).
                const size_t value_indent = required_indent + 1;
                
                if (rest.empty())
                {
                    // Nothing more on this physical line; consume it and expect
                    // the mapping's value on the following line(s).
                    read_line();
                    out_map[key] = parse_block(value_indent);
                }
                else
                {
                    // Value on same line as the key; only a scalar is permitted here.
                    const size_t value_ignore_chars = content_.size() - rest.size();
                    out_map[key] = parse_scalar(value_indent, value_ignore_chars);
                }

                ignore_chars = 0;
            }

            return !out_map.empty();
        }

        // Parses a scalar value starting at the current line, adjusted by
        // `ignore_chars` (chars already consumed by an enclosing construct on
        // this same physical line, e.g. the "key: " before a same-line value).
        Node parse_scalar(size_t min_indent, size_t ignore_chars = 0)
        {           
            char quote;
            bool is_quoted;
            std::string text;
            size_t blank_lines = 0;
            size_t lines_parsed = 0;

            while (!eof_)
            {
                bool line_continuation = false;

                const size_t indent = effective_indent(ignore_chars);
                StringView content = effective_content(ignore_chars);

                if (lines_parsed == 0)
                {
                    if (content.empty())
                        return Node{};

                    quote = content.front();
                    is_quoted = is_quote(quote);

                    if (is_quoted)
                        content.remove_prefix(1); // remove the quote
                }

                read_line();
                ++lines_parsed;

                if (is_quoted)
                {
                    if (parse_quoted_line(content, quote, text, &line_continuation))
                        break; // closing quote found
                }
                else
                {
                    std::string key;
                    StringView rest;
                    if (is_sequence_marker(content) || is_mapping_kvp(content, key, rest))
                        break; // deeper line that's actually a new construct
    
                    text.append(content.data(), content.size());
                }
            
                if (eof_)
                    break;

                if (!is_quoted && !content_.empty() && indent_ < min_indent) // TODO: shouldn't be using indent_ here
                    break; // dedent / sibling: fold ends

                if (content_.empty())
                {
                    ++blank_lines;
                }
                else
                {
                    if (!is_quoted || !line_continuation)
                        append_fold_separator(text, blank_lines);

                    blank_lines = 0;
                }

                ignore_chars = 0;
            }

            return Node(std::move(text));
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

            StringView& s = content; // alias for brevity

            // TODO: We're really trying to parse a scalar here until we find a ":"
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
            // ~TODO ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

        // Appends the separator that joins two folded lines.
        // Line breaks convert to a space " ". However, a series of n blank
        // lines becomes a series of n line breaks.
        static void append_fold_separator(std::string& text, size_t blank_lines)
        {
            text += blank_lines ? std::string(blank_lines, '\n') : " ";
        }

        static bool is_quote(char c)
        {
            return c == '"' || c == '\'';
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
