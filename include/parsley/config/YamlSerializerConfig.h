#pragma once

#include "parsley/config/LineEnding.h"

namespace parsley
{
    struct YamlSerializerConfig
    {
        LineEnding line_endings = LineEnding::LF;
    };
}
