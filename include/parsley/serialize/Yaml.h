#pragma once

#include "parsley/Node.h"
#include "parsley/core/MemoryStreamBuf.h"
#include "parsley/serialize/YamlDeserializer.h"
#include "parsley/serialize/YamlSerializer.h"

namespace parsley
{
    // TODO: consider switching istream/ostream for a lighter alternative

    class Yaml
    {
    public:
        static Node read(std::istream& in)
        {
            return YamlDeserializer{}.read(in);
        }

        static Node read(StringView str)
        {
            MemoryStreamBuf buf(str.data(), str.size());
            std::istream in(&buf);
            return read(in);
        }
    
        static void write(const Node& node, std::ostream& out)
        {
            YamlSerializer{}.write(node, out);
        }
    };
}
