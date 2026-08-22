#pragma once

#include "parsley/Node.h"
#include "parsley/config/YamlDeserializerConfig.h"
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
            read_line();

            skip_structural();

            if (eof_)
                return {}; // no contents -> null

            return parse_block(indent_);
        }

    private:
        // --- Block Parsing --------------------------------------------------

        Node parse_block(size_t min_indent)
        {
            skip_structural();

            if (eof_ || indent_ < min_indent)
                return {}; // nothing here at the required level -> null

            Node node;

            if (try_parse_sequence(indent_, node))
                return node;

            if (try_parse_mapping(indent_, node))
                return node;

            return parse_scalar(indent_);
        }

        bool try_parse_sequence(size_t required_indent, Node& out_seq)
        {
            while (!eof_ && indent_ == required_indent && is_sequence_marker(content_))
            {
                // Parse the key line
                StringView value = content_.substr(1); // remove '-'
                size_t extra = 1 + strip_leading_whitespace(value);
                size_t item_indent = required_indent + extra;
                read_line();

                // Lone '-' without a value on the same line; go to the next line to find a value.     
                if (value.empty())
                {                                   
                    out_seq.push_back(parse_block(item_indent));
                    continue;
                }

                // Found a value on the same line as the key; try to parse mapping or scalar.

                std::string key;
                StringView rest;
                if (is_mapping_kvp(value, key, rest))
                    out_seq.push_back(parse_mapping_lines(item_indent, &key, &rest));
                else
                    out_seq.push_back(parse_scalar_value(value, item_indent));
            }

            return out_seq.size() > 0;
        }

        bool try_parse_mapping(size_t required_indent, Node& out_map)
        {
            std::string key;
            StringView rest;
            if (!is_mapping_kvp(content_, key, rest))
                return false;

            read_line();
            out_map = parse_mapping_lines(required_indent, &key, &rest);
            return true;
        }

        Node parse_scalar(size_t indent)
        {
            StringView content = content_;
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
        // plain or single/double-quoted.
        // Parses the key completely (incl. substitution of line breaks, escape
        // sequences etc.) and returns the unprocessed rest of the line.
        bool is_mapping_kvp(StringView content, std::string& out_key, StringView& out_rest)
        {
            if (content.empty())
                return false;

            StringView& s = content;

            if (is_quote(s.front()))
            {
                const char quote = s.front();
                s.remove_prefix(1); // remove the opening quote

                if (!parse_quoted_line(s, quote, &out_key))
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

        // Parses zero or more "key: value" lines sitting at exactly `indent`.
        // If `first_key`/`first_rest` are given, they're an already-consumed
        // pair (the caller peeled it off a "- key: value" or lookahead line
        // before knowing it was starting a mapping) and get added first,
        // without re-reading a line for it.
        Node parse_mapping_lines(
            size_t key_indent,
            const std::string* first_key = nullptr,
            const StringView* first_rest = nullptr)
        {
            Node map;

            if (first_key)
                add_kvp(map, *first_key, *first_rest, key_indent);

            while (!eof_ && indent_ == key_indent)
            {
                std::string key;
                StringView rest;
                if (!is_mapping_kvp(content_, key, rest))
                    break;

                read_line();
                add_kvp(map, key, rest, key_indent);
            }

            return map;
        }

        void add_kvp(Node& map, const std::string& key, StringView rest, size_t key_indent)
        {
            if (rest.empty())
            {
                map[key] = parse_block(key_indent + 1); // value must be deeper
                return;
            }

            map[key] = parse_scalar_value(rest, key_indent + 1); // value must be deeper
        }

        // --- Scalar Details -------------------------------------------------

        // Cursor must already be advanced past `first_line`'s physical line
        // (every call site does `read_line()` right before calling this).
        // `min_indent` is the minimum column a continuation line must
        // reach to be folded in - see parse_scalar_folded.
        // Single entry point for "read a scalar starting here" - dispatches
        // to the quoted or plain reader and lets either one pull in as many
        // continuation lines as it needs.
        Node parse_scalar_value(StringView first_line, size_t min_indent)
        {
            if (!first_line.empty() && is_quote(first_line[0]))
                return parse_quoted_scalar_value(first_line);

            return parse_scalar_folded(first_line.to_owned(), min_indent);
        }

        // Folds in lines whose indent reaches min_indent or deeper:
        // consecutive lines join with a space, a blank line in the
        // run joins with '\n' instead.
        Node parse_scalar_folded(std::string first_line, size_t min_indent)
        {
            std::string& text = first_line;
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

        // --- Quoted Scalars ---------------------------------------------------

        static bool is_quote(char c)
        {
            return c == '"' || c == '\'';
        }

        // Appends the separator that joins two folded lines: a run of N
        // blank lines becomes N '\n's, otherwise it's a single space.
        // Shared by plain and quoted scalars so they fold identically.
        static void append_fold_separator(std::string& text, size_t blank_lines)
        {
            text += blank_lines ? std::string(blank_lines, '\n') : std::string(" ");
        }

        // Parses a quoted scalar's value, given the text of the opening
        // line (starting at the opening quote). The cursor must already be
        // advanced past that line, same contract as parse_scalar_folded.
        // Continuation lines fold exactly like a plain scalar (blank-line
        // run -> that many '\n', otherwise a single space), except that a
        // double-quoted scalar can end a line with '\' to escape the line
        // break itself, which suppresses the fold separator entirely.
        Node parse_quoted_scalar_value(StringView opening_line)
        {
            const char quote = opening_line.front();
            StringView s = opening_line;
            s.remove_prefix(1);

            std::string text;
            size_t blank_lines = 0;

            for (;;)
            {
                bool line_continuation = false;
                if (parse_quoted_line(s, quote, &text, &line_continuation))
                    break; // closing quote found

                if (eof_)
                    break; // unterminated quote at eof - return what we have

                if (content_.empty())
                {
                    ++blank_lines;
                    read_line();
                    continue;
                }

                if (!line_continuation)
                    append_fold_separator(text, blank_lines);
                blank_lines = 0;

                s = content_;
                read_line();
            }

            return Node(std::move(text));
        }

        // Scans quoted content from `s`, a view into a single physical
        // line, appending unescaped characters to `*parsed` until either
        // the closing `quote` is found or `s` runs out. Returns true once
        // the closing quote is consumed. Returns false if the line ended
        // while still inside the quotes; for a double-quoted scalar ending
        // in a lone trailing '\', *line_continuation is set to true so the
        // caller knows the line break was escaped away, not folded.
        // line_continuation may be null for callers (e.g. is_mapping_kvp)
        // that only care about single-line quoted tokens.
        bool parse_quoted_line(
            StringView& s, 
            char quote, 
            std::string* parsed, 
            bool* line_continuation = nullptr)
        {
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
                        *parsed += '\'';
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

                    parse_escape_sequence(s, *parsed);
                    continue;
                }

                *parsed += c;
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
