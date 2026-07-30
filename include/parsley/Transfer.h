#pragma once

namespace parsley
{
    class Node;

    template <typename T, typename = void>
    struct Transfer
    {
        static void read(const Node& node, T& val);
        static void write(Node& node, const T& val);
    };
}

#include "parsley/detail/Transfer.inl"
