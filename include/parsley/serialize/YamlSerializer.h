#pragma once

#include "parsley/Node.h"

namespace parsley
{
    class YamlSerializer
    {
    public:
        void write(const Node& node, std::ostream& out)
        {
            out_ = &out;
            indent_ = 0;
            
            write_start_marker();            
            write_node(node, /*is_root*/true);

            out_ = nullptr;
        }

    private:
        void write_node(const Node& node, bool is_root)
        {
            if (node.is_null())
                return;

            if (node.is_scalar())
                write_scalar(node, is_root);

            if (node.is_collection())
            {
                if (!is_root)
                    indent_ += 1;

                if (node.is_list()) 
                    write_list(node);
                else
                    write_map(node);

                if (!is_root) 
                    indent_ -= 1;
            }
        }

        void write_scalar(const Node& node, bool is_root)
        {
            write_indent_if_needed();
            *out_ << node.as<StringView>();
            
            if (is_root)
                *out_ << '\n';
        }

        void write_list(const Node& node)
        {
            for (auto entry: node)
            {
                write_indent_if_needed();
                *out_ << "- ";
                at_line_start_ = false; // dash already covers this line's "indent"

                write_node(entry, false);

                if (!entry.value.is_collection())
                    write_newline();
            }
        }

        void write_map(const Node& node)
        {
            for (auto kvp: node)
            {
                write_indent_if_needed();
                *out_ << kvp.key << ":";

                if (kvp.value.is_collection())
                {
                    write_newline();
                }
                else
                {
                    *out_ << ' '; // space already covers this line's "indent"
                    at_line_start_ = false;
                }

                write_node(kvp.value, false);

                if (!kvp.value.is_collection())
                    write_newline();
            }
        }

        void write_start_marker()
        {
            *out_ << "---\n";
            at_line_start_ = true;
        }

        void write_indent_if_needed()
        {
            // Only write indentation if we're at the beginning of a line, 
            // i.e. if the previous step has inserted a line break.
            // This mechanism handles nested lists/maps correctly.

            if (at_line_start_)
            {
                for (size_t i = 0; i < indent_ * 2; ++i)
                    *out_ << ' ';

                at_line_start_ = false;
            }
        }

        void write_newline()
        {
            *out_ << '\n';
            at_line_start_ = true;
        }

        std::ostream* out_;
        int indent_ = 0;
        bool at_line_start_ = true;
    };
}
