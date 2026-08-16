#pragma once

#include "parsley/Node.h"
#include "parsley/config/YamlDeserializerConfig.h"

#include <cassert>
#include <stack>

namespace parsley { namespace detail
{
    template <class Cursor>
    class YamlDeserializer
    {
        struct Frame
        {
            size_t indent;
            bool is_block_start; // true once this frame is known to hold a sequence or mapping
            bool expecting_value; // true if the last marker/key had no value on the same line
            std::string pending_key; // expecting a value for this key
            Node node;
        };

    public:
        YamlDeserializer(YamlDeserializerConfig config = {}) {}

        Node read(Cursor& in)
        {
            // TODO:
            //  - single-line quoted scalars, both ' and " (so `:` inside quotes doesn't split a mapping)
            //  - escape sequences
            //  - multi-line quoted scalars
            //  - inline comments (e.g. `a: b # comment`)
            //  - block scalars (|, >)
            //  - flow style ({...}, [...])
            //  - tags
            //  - anchors/aliases

            indent_ = 0;
            frames_.push({ 0, false, false, {}, Node{} }); // root frame

            // Read the input line by line.
            while (in.get_line(line_))
            {
                handle_leading_whitespace();

                if (should_skip_line())
                    continue;

                if (is_end_marker())
                    break;

                handle_nesting();
                process_content();
            }

            Node root = unwind_frames();
            return root;
        }

    private:
        void handle_leading_whitespace()
        {
            indent_ = get_indent(line_);
            line_.remove_prefix(indent_);
        }

        static size_t get_indent(StringView line)
        {
            size_t indent = 0;
            while (indent < line.size() && line[indent] == ' ')
                ++indent;

            return indent;
        }

        static size_t remove_trailing_whitespace(StringView& line)
        {
            const size_t indent = get_indent(line);
            line.remove_prefix(indent);
            return indent;
        }

        bool should_skip_line()
        {
            // Ignore blank lines/comments/start marker
            return line_.empty() || line_.starts_with("#") || line_.starts_with("---");
        }

        void handle_nesting()
        {
            // Shallower indentation: close nested block(s).
            while (indent_ < frames_.top().indent)
                finalize_top_frame();
            
            if (indent_ > frames_.top().indent)
            {
                // Deeper indentation: indicates the start of a new nested block.

                // We don't know if the new block is a sequence or mapping, so we push a generic frame.
                frames_.push({ indent_, false, false, {}, Node{} });
            }
            else
            {
                // New sibling at the same level: if the previous line was expecting a value, 
                // it's actually null.
                resolve_pending(frames_.top());
            }
        }

        bool is_end_marker()
        {
            return line_.starts_with("...");
        }

        void process_content()
        {
            StringView key;
            StringView value;

            if (is_sequence_marker())
            {
                frames_.top().is_block_start = true;
                process_sequence_node();
            }
            else if (is_mapping_kvp(&key, &value))
            {
                frames_.top().is_block_start = true;
                process_map_node(key, value);
            }
            else
            {
                process_scalar_node();
            }
        }

        bool is_sequence_marker()
        {
            if (line_.empty())
                return false;

            if (line_.size() == 1 && line_.front() == '-')
                return true;

            if (line_.starts_with("- "))
                return true;

            return false;
        }

        bool is_mapping_kvp(StringView* out_key, StringView* out_value)
        {
            for (size_t i = 0; i < line_.size(); ++i)
            {
                if (line_[i] == ':' && (i + 1 == line_.size() || line_[i + 1] == ' '))
                {
                    StringView key = line_.substr(0, i);

                    StringView value = line_.substr(i + 1);
                    remove_trailing_whitespace(value);

                    *out_key = key;
                    *out_value = value;

                    return true;
                }
            }

            return false;
        }

        void process_sequence_node()
        {            
            // strip "-" and any following spaces
            line_.remove_prefix(1);
            const size_t extra_indent = 1 + remove_trailing_whitespace(line_);

            if (line_.empty())
            {
                // Line ended without a value, expect it on the next line.
                frames_.top().expecting_value = true;
                return;
            }

            StringView key;
            StringView value;
            if (is_mapping_kvp(&key, &value))
            {
                // "- key: value": the sequence item's value is itself a mapping. 
                // Open a frame anchored at the key's column so later lines aligned 
                // with it are treated as siblings of this map, not as a new sequence entry.
                frames_.push({ indent_ + extra_indent, true, false, {}, Node{} });

                process_map_node(key, value);
                return;
            }
            
            // Plain scalar entry, push the value.
            frames_.top().node.push_back(Node(line_.to_owned()));
        }

        void process_map_node(StringView key, StringView value)
        {
            assert(!key.empty()); // key should never be empty

            if (value.empty())
            {
                // Key is present, but value is missing. We expect the value on the next lines.
                frames_.top().expecting_value = true;
                frames_.top().pending_key = key.to_owned();
                return;
            }

            // Key and value are both present, insert them.
            frames_.top().node[key.to_owned()] = Node(value.to_owned());
        }

        void process_scalar_node()
        {
            frames_.top().node = Node(line_.to_owned());
        }

        Node unwind_frames()
        {
            // Unwind any remaining open levels (except for the root node)
            while (frames_.size() > 1)
                finalize_top_frame();

            // Resolve the root node (e.g. root sequence of `null`)
            resolve_pending(frames_.top());
            Node root = std::move(frames_.top().node);
            frames_.pop();

            return root;
        }

        // TODO: this and its nested functions need a refactor
        void finalize_top_frame()
        {
            Frame finished = std::move(frames_.top());
            frames_.pop();

            resolve_pending(finished);
            attach(std::move(finished));
        }

        static void resolve_pending(Frame& f)
        {
            if (!f.expecting_value)
                return;

            if (!f.pending_key.empty())
                f.node[f.pending_key] = Node();
            else
                f.node.push_back(Node());

            f.expecting_value = false;
            f.pending_key = {};
        }

        // Attach a just-closed child block to its parent: as a pending key's value,
        // or as the next sequence entry.
        void attach(Frame&& finished)
        {
            if (!finished.is_block_start)
                return;

            Frame& parent = frames_.top();

            if (parent.expecting_value && !parent.pending_key.empty())
                parent.node[parent.pending_key] = std::move(finished.node);
            else
                parent.node.push_back(std::move(finished.node));

            parent.expecting_value = false;
            parent.pending_key = {};
        }

        std::stack<Frame> frames_;
        StringView line_;
        size_t indent_;
    };
}}
