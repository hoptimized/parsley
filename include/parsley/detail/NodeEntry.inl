#pragma once

// This include helps clangd understand the code
#include "parsley/Node.h"

namespace parsley
{
    template <typename NodeRef>
    Node::EntryBase<NodeRef>::EntryBase(StringView key, NodeRef value) : key(key), value(value)
    {}

    template <typename NodeRef>
    template<typename OtherNodeRef, typename>
    Node::EntryBase<NodeRef>::EntryBase(const Node::EntryBase<OtherNodeRef>& other) : 
        key(other.key), 
        value(other.value)
    {}

    template <typename NodeRef>
    Node::EntryBase<NodeRef>::operator NodeRef()
    {
        return value;
    }

    template <typename NodeRef>
    template <typename T, typename, typename>
    Node::EntryBase<NodeRef>& Node::EntryBase<NodeRef>::operator=(T&& val)
    {
        value = std::move(val);
        return *this;
    }
}
