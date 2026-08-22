#pragma once

#include "parsley/Node.h"
#include "parsley/config/YamlDeserializerConfig.h"

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

            skip_structural_noise();

            if (eof())
                return {};

            return parse_block(current_indent());
        }

    private:
        Node parse_block(size_t min_indent)
        {
            skip_structural_noise();

            if (eof() || current_indent() < min_indent)
                return {}; // nothing here at this level -> null

            size_t indent = current_indent();
            StringView content = current_content();
            
            Node node;

            if (try_parse_sequence(content, indent, &node))
                return node;

            if (try_parse_mapping(content, indent, &node))
                return node;

            return parse_scalar(indent);
        }

        bool try_parse_sequence(StringView content, size_t indent, Node* out_seq)
        {
            if (!is_sequence_marker(content))
                return false;

            while (!eof() && current_indent() == indent && is_sequence_marker(current_content()))
            {
                StringView content = current_content();
                content.remove_prefix(1); // '-'
                size_t extra = 1 + strip_leading_whitespace(content);
                size_t item_indent = indent + extra;

                if (content.empty())
                {
                    read_line();
                    out_seq->push_back(parse_block(item_indent));
                    continue;
                }

                StringView key, rest;
                if (is_mapping_kvp(content, &key, &rest))
                {
                    read_line();
                    Node map;
                    parse_mapping_lines(map, item_indent, &key, &rest);
                    out_seq->push_back(std::move(map));
                }
                else
                {
                    std::string text = content.to_owned();
                    read_line();
                    out_seq->push_back(parse_scalar_folded(std::move(text), item_indent - 1));
                }
            }

            return out_seq->size() > 0;
        }

        bool try_parse_mapping(StringView content, size_t indent, Node* out_map)
        {
            StringView key, rest;
            if (!is_mapping_kvp(current_content(), &key, &rest))
                return false;

            read_line();
            parse_mapping_lines(*out_map, indent, &key, &rest);
            return true;
        }

        // Parses zero or more "key: value" lines sitting at exactly `indent`.
        // If `first_key`/`first_rest` are given, they're an already-consumed
        // pair (the caller peeled it off a "- key: value" or lookahead line
        // before knowing it was starting a mapping) and get added first,
        // without re-reading a line for it.
        void parse_mapping_lines(
            Node& out_map,
            size_t indent,
            const StringView* first_key = nullptr,
            const StringView* first_rest = nullptr)
        {
            if (first_key)
                add_kvp(out_map, *first_key, *first_rest, indent);

            while (!eof() && current_indent() == indent)
            {
                StringView key, rest;
                if (!is_mapping_kvp(current_content(), &key, &rest))
                    break;

                read_line();
                add_kvp(out_map, key, rest, indent);
            }
        }

        void add_kvp(Node& map, StringView key, StringView rest, size_t key_indent)
        {
            if (rest.empty())
                map[key.to_owned()] = parse_block(key_indent + 1); // value must be deeper
            else
                map[key.to_owned()] = parse_scalar_folded(rest.to_owned(), key_indent);
        }

        Node parse_scalar(size_t indent)
        {
            std::string text = current_content().to_owned();
            read_line();
            return parse_scalar_folded(std::move(text), indent);
        }

        // Folds in following lines indented deeper than min_indent: 
        // consecutive lines join with a space, a blank line in the 
        // run joins with '\n' instead.
        Node parse_scalar_folded(std::string first_line, size_t min_indent)
        {
            std::string& text = first_line;
            size_t blank_lines = 0;

            while (!eof())
            {
                if (current_line_is_blank())
                {
                    ++blank_lines;
                    read_line();
                    continue;
                }

                if (current_indent() <= min_indent)
                    break; // dedent / sibling: fold ends

                StringView content = current_content();
                StringView key, rest;
                if (is_sequence_marker(content) || is_mapping_kvp(content, &key, &rest))
                    break; // deeper line that's actually a new construct

                text += blank_lines ? std::string(blank_lines, '\n') : " ";
                blank_lines = 0;
                text += content.to_owned();
                read_line();
            }

            return Node(std::move(text));
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

        // TODO: not quote-aware - `"a: b": c` misparses.
        bool is_mapping_kvp(StringView content, StringView* out_key, StringView* out_rest)
        {
            for (size_t i = 0; i < content.size(); ++i)
            {
                if (content[i] == ':' && (i + 1 == content.size() || content[i + 1] == ' '))
                {
                    *out_key = content.substr(0, i);
                    StringView rest = content.substr(i + 1);
                    strip_leading_whitespace(rest);
                    *out_rest = rest;
                    return true;
                }
            }
            return false;
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

        void skip_structural_noise()
        {
            while (!eof())
            {
                StringView t = line_;
                strip_leading_whitespace(t);

                if (t.empty() || t.starts_with("#") || t.starts_with("---"))
                {
                    read_line();
                    continue;
                }

                if (t.starts_with("..."))
                {
                    eof_ = true;
                    return;
                }

                return;
            }
        }

        bool current_line_is_blank()
        {
            StringView t = line_;
            strip_leading_whitespace(t);
            return t.empty();
        }

        size_t current_indent()
        {
            return get_indent(line_);
        }

        StringView current_content()
        {
            StringView line = line_;
            line.remove_prefix(current_indent());
            return line;
        }

        void read_line()
        {
            eof_ = !in_->get_line(line_);
        }

        bool eof() const
        {
            return eof_;
        }

        Cursor* in_;
        StringView line_;
        bool eof_ = false;
    };
}}
