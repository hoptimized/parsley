#pragma once

#include <ostream>
#include <string>

#include "parsley/core/StringView.h"

namespace parsley
{    class StringOutputCursor
    {
    public:
        explicit StringOutputCursor(std::string& str) : str_(str) {}

        void write(StringView text)
        {
            str_.append(text.data(), text.size());
        }

    private:
        std::string& str_;
    };

    class StreamOutputCursor
    {
    public:
        explicit StreamOutputCursor(std::ostream& stream) : out_(stream) {}

        void write(StringView text)
        {
            out_.write(text.data(), text.size());
        }

    private:
        std::ostream& out_;
    };
}
