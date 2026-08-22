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
        Node parse_block(size_t min_indent)
        {
            skip_structural();

            if (eof_ || indent_ < min_indent)
                return {}; // nothing here at the required level -> null

            Node node;

            if (try_parse_sequence(indent_, &node))
                return node;

            if (try_parse_mapping(indent_, &node))
                return node;

            return parse_scalar(indent_);
        }

        bool try_parse_sequence(size_t required_indent, Node* out_seq)
        {
            while (!eof_ && indent_ == required_indent && is_sequence_marker(content_))
            {
                StringView value = content_.substr(1); // remove '-'
                size_t extra = 1 + strip_leading_whitespace(value);
                size_t item_indent = required_indent + extra;

                if (value.empty())
                {
                    // Lone '-' without a value on the same line.
                    // Go to the next line to find a value.
                    read_line();
                    out_seq->push_back(parse_block(item_indent));
                    continue;
                }

                std::string key;
                StringView rest;
                if (is_mapping_kvp(value, &key, &rest))
                {
                    read_line();
                    Node map = parse_mapping_lines(item_indent, &key, &rest);
                    out_seq->push_back(std::move(map));
                }
                else if (is_quote(value[0]))
                {
                    // TODO: do we really need a completely separate function for quoted?
                    // TODO: this doesn't fold.
                    Node scalar = parse_quoted_scalar_value(value);
                    read_line();
                    out_seq->push_back(std::move(scalar));
                }
                else
                {
                    std::string text = value.to_owned();
                    read_line();
                    out_seq->push_back(parse_scalar_folded(std::move(text), item_indent - 1));
                }
            }

            return out_seq->size() > 0;
        }

        bool try_parse_mapping(size_t required_indent, Node* out_map)
        {
            std::string key;
            StringView rest;
            if (!is_mapping_kvp(content_, &key, &rest))
                return false;

            read_line();
            *out_map = parse_mapping_lines(required_indent, &key, &rest);
            return true;
        }

        // Parses zero or more "key: value" lines sitting at exactly `indent`.
        // If `first_key`/`first_rest` are given, they're an already-consumed
        // pair (the caller peeled it off a "- key: value" or lookahead line
        // before knowing it was starting a mapping) and get added first,
        // without re-reading a line for it.
        Node parse_mapping_lines(
            size_t required_indent,
            const std::string* first_key = nullptr,
            const StringView* first_rest = nullptr)
        {
            Node map;

            if (first_key)
                add_kvp(map, *first_key, *first_rest, required_indent);

            while (!eof_ && indent_ == required_indent)
            {
                std::string key;
                StringView rest;
                if (!is_mapping_kvp(content_, &key, &rest))
                    break;

                read_line();
                add_kvp(map, key, rest, required_indent);
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

            if (is_quote(rest[0]))
            {
                map[key] = parse_quoted_scalar_value(rest);
                return;
            }

            map[key] = parse_scalar_folded(rest.to_owned(), key_indent);
        }

        Node parse_scalar(size_t indent)
        {
            StringView content = content_;

            if (!content.empty() && is_quote(content[0]))
            {
                Node node = parse_quoted_scalar_value(content);
                read_line();
                return node;
            }

            std::string text = content.to_owned();
            read_line();
            return parse_scalar_folded(std::move(text), indent);
        }

        // Folds in following lines indented deeper than min_indent:
        // consecutive lines join with a space, a blank line in the
        // run joins with '\n' instead.
        //
        // Only used for PLAIN scalars - quoted scalars go through
        // parse_quoted_scalar_value instead and don't fold (yet).
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

                if (indent_ <= min_indent)
                    break; // dedent / sibling: fold ends

                StringView content = content_;
                std::string key;
                StringView rest;
                if (is_sequence_marker(content) || is_mapping_kvp(content, &key, &rest))
                    break; // deeper line that's actually a new construct

                text += blank_lines ? std::string(blank_lines, '\n') : " ";
                blank_lines = 0;
                text += content.to_owned();
                read_line();
            }

            return Node(std::move(text));
        }

        // --- quoted scalars ---------------------------------------------------

        static bool is_quote(char c)
        {
            return c == '"' || c == '\'';
        }

        // TODO: currently doesn't fold multi-line quoted scalars
        Node parse_quoted_scalar_value(StringView content)
        {
            std::string text;
            parse_quoted_body(content, &text);
            return Node(std::move(text));
        }

        // Consumes a quoted token from the start of `s` (s[0] must be a
        // quote character). Advances `s` past the closing quote on success.
        // Returns false if no closing quote was found on this line.
        bool parse_quoted_body(StringView& s, std::string* parsed)
        {
            // Read and consume quote to identify type (single-quoted vs. double-quoted)
            const char quote = s.front();
            s.remove_prefix(1);

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

                // Escape sequences
                if (quote == '"' && c == '\\' && s.size() > 1)
                {
                    parse_escape_sequence(s, *parsed);
                    continue;
                }

                *parsed += c;
                s.remove_prefix(1);
            }

            return false;
        }

        // --- line helpers ---------------------------------------------------

        bool is_sequence_marker(StringView content)
        {
            if (content.empty())
                return false;

            if (content.size() == 1 && content.front() == '-')
                return true;

            return content.starts_with("- ");
        }

        // Detects "key: rest" at the start of `content`, where key may be
        // plain or single/double-quoted. A colon inside a quoted key does
        // NOT end the key (that's the whole reason this needs to be
        // quote-aware, vs. just scanning for the first ':').
        bool is_mapping_kvp(StringView content, std::string* out_key, StringView* out_rest)
        {
            if (content.empty())
                return false;

            StringView s = content;
            std::string key;

            if (is_quote(s[0]))
            {
                if (!parse_quoted_body(s, &key))
                    return false; // unterminated quote on this line
            }
            else
            {
                size_t i = 0;
                for (; i < s.size(); ++i)
                {
                    if (s[i] == ':' && (i + 1 == s.size() || s[i + 1] == ' '))
                        break;
                }
                if (i == s.size())
                    return false; // no colon marker found

                key = s.substr(0, i).to_owned();
                s.remove_prefix(i);
            }

            // s now starts right where the key token ended - must be ':'
            // followed by a space or end of line to count as a mapping marker.
            if (s.empty() || s[0] != ':')
                return false;
            if (s.size() > 1 && s[1] != ' ')
                return false;

            s.remove_prefix(1); // ':'
            strip_leading_whitespace(s);

            *out_key = std::move(key);
            *out_rest = s;
            return true;
        }

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

        Cursor* in_;
        StringView line_;
        size_t indent_;
        StringView content_;
        bool eof_ = false;
    };
}}
