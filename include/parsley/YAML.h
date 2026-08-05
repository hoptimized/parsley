#pragma once

#include "parsley/detail/YamlDeserializer.h"
#include "parsley/detail/YamlSerializer.h"

namespace parsley
{
    class YAML
    {
    public:
        template <class Cursor>
        using Serializer = detail::YamlSerializer<Cursor>;

        using SerializerConfig = YamlSerializerConfig;

        template <class Cursor>
        using Deserializer = detail::YamlDeserializer<Cursor>;

        using DeserializerConfig = YamlDeserializerConfig;
    };
}
