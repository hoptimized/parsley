#pragma once

#include "parsley/core/StringView.h"

#include <streambuf>

namespace parsley
{
    struct StringViewInputBuffer final : std::streambuf
    {
        StringViewInputBuffer(StringView sv)
        {
            char* p = const_cast<char*>(sv.data());
            setg(p, p, p + sv.size());
        }

    protected:
        std::streamsize xsputn(const char*, std::streamsize) override { return 0; }
        int_type overflow(int_type) override { return traits_type::eof(); }
    };
}
