#pragma once

#include "parsley/Node.h"

#include <stack>

namespace parsley
{
    struct YamlDeserializerConfig
    {
    };

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
            //  - inline comments (e.g. `a: b # comment`)
            //  - quoted scalars (so `:` inside quotes doesn't split a mapping)
            //  - end marker `...`
            //  - flow style ({...}, [...])
            //  - anchors/aliases
            //  - block scalars (|, >)

            std::stack<Frame> frames;
            frames.push({ 0, false, false, {}, Node{} }); // root frame

            // Read the input line by line.
            for (StringView line; in.get_line(line);)
            {
                size_t indent = get_indent(line);
                StringView content = StringView{ line }.substr(indent);

                // Ignore blank lines/comments/start marker
                if (content.empty() || content.starts_with("#") || content.starts_with("---"))
                    continue;

                // Shallower indentation: close nested block(s).
                while (indent < frames.top().indent)
                {
                    Frame finished = std::move(frames.top());
                    frames.pop();
                    resolve_pending(finished);
                    attach(frames.top(), std::move(finished));
                }
                
                if (indent > frames.top().indent)
                {
                    // Deeper indentation: this indicates the start of a new nested block.
                    // We don't know if the new block is a sequence or mapping, so we push a generic frame.
                    frames.push({ indent, false, false, {}, Node{} });
                }
                else
                {
                    // Sibling at the same level: anything pending was actually null.
                    resolve_pending(frames.top());
                }

                // --- Handle content ---------

                StringView key;
                StringView value;

                if (is_sequence_marker(content))
                {
                    // Now that we're certain this is a sequence, start a block.
                    frames.top().is_block_start = true;
                    
                    // strip "-" and any following spaces
                    size_t extra_indent = 1;
                    content = content.substr(1);
                    while (content.starts_with(" "))
                    {
                        content.remove_prefix(1);
                        ++extra_indent;
                    }

                    if (content.empty())
                    {
                        // Line ended without a value, expect it on the next line.
                        frames.top().expecting_value = true;
                    }
                    else if (is_mapping_kvp(content, &key, &value))
                    {
                        // "- key: value": the sequence item's value is itself a mapping. 
                        // Open a frame anchored at the key's column so later lines aligned 
                        // with it are treated as siblings of this map, not as a new sequence entry.
                        frames.push({ indent + extra_indent, true, false, {}, Node{} });

                        if (!value.empty())
                        {
                            frames.top().node[key.to_owned()] = Node(value.to_owned());
                        }
                        else
                        {
                            frames.top().expecting_value = true;
                            frames.top().pending_key = key.to_owned();
                        }
                    }
                    else
                    {
                        // Plain scalar entry, push the value.
                        frames.top().node.push_back(Node(content.to_owned()));
                    }
                }
                else if (is_mapping_kvp(content, &key, &value))
                {
                    // Now that we're certain this is a mapping, start a block.
                    frames.top().is_block_start = true;

                    if (!value.empty())
                    {
                        // Key and value are both present, insert them.
                        frames.top().node[key.to_owned()] = Node(value.to_owned());
                    }
                    else
                    {
                        // Key is present, but value is expected on the next line.
                        frames.top().expecting_value = true;
                        frames.top().pending_key = key.to_owned();
                    }
                }
                else
                {
                    // Plain scalar entry
                    frames.top().node = Node(content.to_owned());
                }
            }

            // Unwind any remaining open levels (except for the root node)
            while (frames.size() > 1)
            {
                Frame finished = std::move(frames.top());
                frames.pop();
                resolve_pending(finished);
                attach(frames.top(), std::move(finished));
            }

            // Resolve the root node (e.g. root sequence of `null`)
            resolve_pending(frames.top());

            return std::move(frames.top().node);
        }

    private:
        static size_t get_indent(StringView line)
        {
            std::size_t indent = 0;
            while (indent < line.size() && line[indent] == ' ')
                ++indent;

            return indent;
        }

        static bool is_sequence_marker(StringView content)
        {
            if (content.empty())
                return false;

            if (content.size() == 1 && content.front() == '-')
                return true;

            if (content.starts_with("- "))
                return true;

            return false;
        }

        static bool is_mapping_kvp(StringView content, StringView* out_key, StringView* out_value)
        {
            for (size_t i = 0; i < content.size(); ++i)
            {
                if (content[i] == ':' && (i + 1 == content.size() || content[i + 1] == ' '))
                {
                    StringView key = content.substr(0, i);

                    StringView value = content.substr(i + 1);
                    while (value.starts_with(" "))
                        value.remove_prefix(1);

                    *out_key = key;
                    *out_value = value;

                    return true;
                }
            }

            return false;
        }

        static void resolve_pending(Frame& f)
        {
            if (!f.expecting_value)
            {
                // If we reach here still expecting a value, no deeper block ever showed up -> null.
                return;
            }

            if (!f.pending_key.empty())
                f.node[f.pending_key] = Node();
            else
                f.node.push_back(Node());

            f.expecting_value = false;
            f.pending_key = {};
        }

        // Attach a just-closed child block to its parent: as a pending key's value,
        // or as the next sequence entry.
        static void attach(Frame& parent, Frame&& finished)
        {
            if (!finished.is_block_start)
                return;

            if (parent.expecting_value)
            {
                if (!parent.pending_key.empty())
                    parent.node[parent.pending_key] = std::move(finished.node);
                else
                    parent.node.push_back(std::move(finished.node));

                parent.expecting_value = false;
                parent.pending_key = {};
            }
            else
            {
                parent.node.push_back(std::move(finished.node));
            }
        }
    };
}
