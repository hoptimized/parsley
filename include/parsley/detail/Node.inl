#pragma once

// This include helps clangd understand the code
#include "parsley/Node.h"

#include "parsley/Transfer.h"
#include "parsley/detail/util.h"

#include <stdexcept>

namespace parsley
{
    inline Node::Node() = default;    
    inline Node::Node(Node&&) = default;
    inline Node& Node::operator=(Node&&) = default;

    //-------------------------------------------------------------------------------------------------
    // Construction & Assignment

    template <typename T, typename>
    inline Node::Node(T&& val)
    {
        Transfer<decay_t<T>>::write(*this, std::forward<T>(val));
    }

    template <typename T>
    inline Node& Node::operator=(T&& val)
    {
        Transfer<decay_t<T>>::write(*this, std::forward<T>(val));
        return *this;
    }

    //-------------------------------------------------------------------------------------------------
    // Modifiers

    template <typename T>
    inline void Node::push_back(T&& val)
    {
        using namespace detail;

        if (!is_list())
        {
            if (is_null())
            {
                storage_ = ListStorage{};
            }
            else 
            {
                throw std::runtime_error("Trying to push to invalid node type.");
            }
        }
        
        auto child = make_unique<Node>();
        *child = std::forward<T>(val);

        auto& values = storage_.get<ListStorage>().values;
        values.emplace_back(std::move(child));
    }

    inline void Node::clear()
    {
        storage_ = detail::NullStorage{};
    }

    //-------------------------------------------------------------------------------------------------
    // Access

    template <typename T>
    inline T Node::as() const
    {
        T val{};
        Transfer<T>::read(*this, val);
        return val;
    }

    template <typename T>
    inline Node& Node::operator[](T key) 
    {
        return *get_collection_node(key, /*allow_insert=*/true);
    }

    template <typename T>
    inline const Node& Node::operator[](T key) const 
    {
        return *const_cast<Node*>(this)->get_collection_node(key, /*allow_insert=*/false);
    }

    inline Node::Iterator Node::begin()
    {
        if (is_list())
            return { NodeType::List, storage_.get<detail::ListStorage>().values.begin(), {} };
        
        if (is_map())
            return { NodeType::Map, {}, storage_.get<detail::MapStorage>().kvps.begin() };

        return { type(), {}, {} };
    }

    inline Node::Iterator Node::end()
    {
        if (is_list())
            return { NodeType::List, storage_.get<detail::ListStorage>().values.end(), {} };
        
        if (is_map())
            return { NodeType::Map, {}, storage_.get<detail::MapStorage>().kvps.end() };

        return { type(), {}, {} };
    }

    inline Node::ConstIterator Node::begin() const
    {
        return const_cast<Node*>(this)->begin();
    }

    inline Node::ConstIterator Node::end() const
    {
        return const_cast<Node*>(this)->end();
    }

    inline Node::ConstIterator Node::cbegin() const
    {
        return begin();
    }

    inline Node::ConstIterator Node::cend() const
    {
        return end();
    }

    //-------------------------------------------------------------------------------------------------
    // Identity

    inline NodeType Node::type() const
    { 
        return storage_.type(); 
    }

    inline bool Node::is_null() const
    { 
        return storage_.is<detail::NullStorage>(); 
    }

    inline bool Node::is_scalar() const
    {
        return storage_.is<detail::ScalarStorage>();
    }

    inline bool Node::is_list() const
    {
        return storage_.is<detail::ListStorage>();
    }

    inline bool Node::is_map() const
    {
        return storage_.is<detail::MapStorage>();
    }

    inline bool Node::is_collection() const
    {
        return is_list() || is_map();
    }

    inline bool Node::is(NodeType type) const
    {
        return storage_.is(type);
    }

    template <typename T>
    struct Node::EqualityComparer<T, typename std::enable_if<std::is_convertible<T, StringView>::value>::type>
    {
        static bool equals(const Node& node, const StringView other)
        {
            return node.get_scalar() == other;
        }
    };

    template <>
    struct Node::EqualityComparer<Node>
    {
        static bool equals(const Node& node, const Node& other)
        {
            return node.storage_ == other.storage_;
        }
    };

    //-------------------------------------------------------------------------------------------------
    // Capacity

    inline bool Node::empty() const
    {
        return size() == 0;
    }

    inline std::size_t Node::size() const
    {
        using namespace detail;

        if (const auto* null = storage_.try_get<NullStorage>())
            return 0;
        if (const auto* scalar = storage_.try_get<ScalarStorage>())
            return 1;
        if (const auto* list = storage_.try_get<ListStorage>())
            return list->values.size();
        if (const auto* map = storage_.try_get<MapStorage>())
            return map->kvps.size();
        return 0;
    }

    //-------------------------------------------------------------------------------------------------
    // Low-level access

    inline void Node::set_scalar(std::string s)
    {
        storage_ = detail::ScalarStorage{ std::move(s) };
    }

    inline const std::string& Node::get_scalar() const
    {
        const auto* s = storage_.try_get<detail::ScalarStorage>();
        if (!s)
            throw std::runtime_error("Node does not hold a scalar");
        return s->value;
    }

    inline Node* Node::get_collection_node(size_t idx, bool allow_insert)
    {
        using namespace detail;

        if (is_scalar())
            throw std::runtime_error("Cannot access scalar by key.");

        if (is_map())
            throw std::runtime_error("Cannot access map by integral key.");

        // Try convert Null node to List
        if (is_null()) 
        {
            if (!allow_insert) 
                throw std::runtime_error("Cannot access null node by key.");

            if (idx > 0)
                throw std::runtime_error("Cannot skip index when inserting.");

            storage_ = ListStorage{};
        }

        // We are now certain that this is a List, continue to lookup/insert

        auto& values = storage_.get<ListStorage>().values;

        // Index within bounds, access List by index
        if (idx < values.size())
            return values[idx].get();

        // Index is bounds+1, try insert new node
        if (idx == values.size() && allow_insert) 
        {
            values.emplace_back(make_unique<Node>());
            return values.back().get();
        }

        throw std::runtime_error("Index out of bounds.");
    }

    inline Node* Node::get_collection_node(StringView key, bool allow_insert)
    {
        using namespace detail;

        if (is_scalar())
            throw std::runtime_error("Cannot access scalar by key.");

        // Try convert Null node to Map
        if (is_null())
        {
            if (!allow_insert) 
                throw std::runtime_error("Cannot access null node by key.");

            storage_ = MapStorage{};
        }

        // Try convert List to Map
        if (is_list())
        {
            if (!allow_insert)
                throw std::runtime_error("Cannot access list by string key.");

            auto old_values = std::move(storage_.get<ListStorage>().values);

            storage_ = MapStorage{};
            auto& kvps = storage_.get<MapStorage>().kvps;

            for (size_t i = 0; i < old_values.size(); ++i)
            {
                kvps.emplace_back(
                    std::to_string(i),
                    std::move(old_values[i])
                );
            }
        }

        // Access Map by key or insert new element
        if (is_map())
        {
            if (auto value = find_map_value(key))
                return value; // key found
            
            if (allow_insert)
            {
                auto& kvps = storage_.get<MapStorage>().kvps;
                kvps.emplace_back(std::make_pair(key.to_owned(), make_unique<Node>()));
                return kvps.back().second.get();
            }

            throw std::runtime_error("Key does not exist.");
        }

        throw std::runtime_error("Unhandled.");
    }

    inline const Node* Node::find_map_value(StringView key) const
    {
        const auto& kvps = storage_.get<detail::MapStorage>().kvps;

        for (const auto& kvp: kvps)
        {
            if (kvp.first == key)
                return kvp.second.get();
        }

        return nullptr;
    }

    inline Node* Node::find_map_value(StringView key)
    {
        return const_cast<Node*>(static_cast<const Node&>(*this).find_map_value(key));
    }
}
