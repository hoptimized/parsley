#pragma once

// This include helps clangd understand the code
#include "parsley/Node.h"

namespace parsley
{
    template<typename ListIter, typename MapIter, typename EntryT>
    Node::IteratorBase<ListIter, MapIter, EntryT>::IteratorBase(
        NodeType type,
        ListIter list_it,
        MapIter map_it)
    :
        type_(type),
        list_it_(list_it),
        map_it_(map_it)
    {}

    template<typename ListIter, typename MapIter, typename EntryT>
    template<typename OtherListIter, typename OtherMapIter, typename OtherEntryT, typename>
    Node::IteratorBase<ListIter, MapIter, EntryT>::IteratorBase(
        const Node::IteratorBase<OtherListIter, OtherMapIter, OtherEntryT>& other) 
    : 
        type_(other.type_), 
        list_it_(other.list_it_), 
        map_it_(other.map_it_)
    {}

    template<typename ListIter, typename MapIter, typename EntryT>
    EntryT Node::IteratorBase<ListIter, MapIter, EntryT>::operator*() const
    {
        switch (type_)
        {
            case NodeType::List:
                return EntryT{ {}, **list_it_ };
            case NodeType::Map:
                return EntryT{ map_it_->first, *map_it_->second };
            default:
                throw std::runtime_error("Cannot dereference end iterator.");
        }
    }

    template<typename ListIter, typename MapIter, typename EntryT>
    Node::IteratorBase<ListIter, MapIter, EntryT>& Node::IteratorBase<ListIter, MapIter, EntryT>::operator++()
    {
        switch (type_)
        {
            case NodeType::List:
                ++list_it_;
                break;
            case NodeType::Map:
                ++map_it_;
                break;
            default:
                break;
        }
        return *this;
    }

    template<typename ListIter, typename MapIter, typename EntryT>
    Node::IteratorBase<ListIter, MapIter, EntryT> Node::IteratorBase<ListIter, MapIter, EntryT>::operator++(int)
    { 
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    template<typename ListIter, typename MapIter, typename EntryT>
    template<typename OtherListIter, typename OtherMapIter, typename OtherEntryT>
    bool Node::IteratorBase<ListIter, MapIter, EntryT>::operator==(
        const Node::IteratorBase<OtherListIter, OtherMapIter, OtherEntryT>& other) const
    {
        if (type_ != other.type_)
            return false;

        switch (type_)
        {
            case NodeType::List: return list_it_ == other.list_it_;
            case NodeType::Map:  return map_it_ == other.map_it_;
            default: return true;
        }
    }

    template<typename ListIter, typename MapIter, typename EntryT>
    template<typename OtherListIter, typename OtherMapIter, typename OtherEntryT>
    bool Node::IteratorBase<ListIter, MapIter, EntryT>::operator!=(
        const Node::IteratorBase<OtherListIter, OtherMapIter, OtherEntryT>& other) const
    {
        return !(*this == other);
    }
}
