#pragma once

#include "parsley/config/LineEnding.h"

namespace parsley
{
    struct YamlSerializerConfig
    {
        LineEnding line_endings = LineEnding::LF;
        bool write_start_marker = true;
        bool write_end_marker = false;
    };
}
