#pragma once

#include "parsley/serialize/YamlDeserializer.h"
#include "parsley/serialize/YamlSerializer.h"

namespace parsley
{
    class YAML
    {
    public:
        template <class Cursor>
        using Serializer = YamlSerializer<Cursor>;

        using SerializerConfig = YamlSerializerConfig;

        template <class Cursor>
        using Deserializer = YamlDeserializer<Cursor>;

        using DeserializerConfig = YamlDeserializerConfig;
    };
}
