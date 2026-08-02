#pragma once

#include "parsley/serialize/Format.h"
#include "parsley/serialize/YamlSerializer.h"

#include <stdexcept>
#include <string>

namespace parsley
{
    // TODO: consider this alternative interface
    //template <typename SerializerT>
    //static std::string write(const Node& node, SerializerT serializer = {})
    //{
    //    std::ostringstream oss;
    //    SerializerT{}.write(node, oss);
    //    return oss.str();
    //}

    template <typename SerializerT>
    static std::string serialize(const Node& node)
    {
        std::ostringstream oss;
        SerializerT{}.write(node, oss);
        return oss.str();
    }

    static std::string serialize(const Node& node, Format format)
    {
        switch (format)
        {
            case Format::YAML:
                return serialize<YamlSerializer>(node);
            default:
                throw std::runtime_error("Unsupported format.");
        }
    }
}
