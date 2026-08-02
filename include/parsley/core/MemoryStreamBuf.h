#pragma once

#include "parsley/core/StringView.h"

namespace parsley
{
    class MemoryStreamBuf : public std::streambuf 
    {
    public:
        MemoryStreamBuf(const char* data, size_t size) 
        {
            char* p = const_cast<char*>(data);
            setg(p, p, p + size);
        }

        explicit MemoryStreamBuf(StringView str) 
            : MemoryStreamBuf(str.data(), str.size()) 
        {}
    };
}
