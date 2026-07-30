#pragma once

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
        Transfer<std::decay_t<T>>::write(*this, std::forward<T>(val));
    }

    template <typename T>
    inline Node& Node::operator=(T&& val)
    {
        Transfer<std::decay_t<T>>::write(*this, std::forward<T>(val));
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
        
        auto child = std::make_unique<Node>();
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

    inline bool Node::is(NodeType type) const
    {
        return storage_.is(type);
    }

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

    template <typename T>
    inline Node* Node::get_collection_node(T key, bool allow_insert)
    {
        using namespace detail;
        using U = remove_cvref_t<T>;

        if (is_scalar())
            throw std::runtime_error("Cannot access scalar by key.");

        if constexpr (std::is_integral_v<U>) 
        {
            if (is_map())
                throw std::runtime_error("Cannot access map by integral key.");

            std::size_t idx = to_index(key);

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
                values.emplace_back(std::make_unique<Node>());
                return values.back().get();
            }

            throw std::runtime_error("Index out of bounds.");
        }
        else if constexpr (std::is_constructible_v<std::string_view, T>)
        {
            std::string_view str_key{ key };

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
                if (auto value = find_map_value(str_key))
                    return value; // key found
                
                if (allow_insert)
                {
                    auto& kvps = storage_.get<MapStorage>().kvps;
                    kvps.emplace_back(std::make_pair(str_key, std::make_unique<Node>()));
                    return kvps.back().second.get();
                }

                throw std::runtime_error("Key does not exist.");
            }
        }
        else
        {
            static_assert([] { return false; }(), "Unsupported key type");
        }

        throw std::runtime_error("Unhandled.");
    }

    template <typename T>
    inline size_t Node::to_index(T key)
    {
        using U = remove_cvref_t<T>;

        if constexpr (std::is_signed_v<U>)
        {
            if (key < 0)
                throw std::out_of_range("Cannot access list by negative index");

            return static_cast<std::size_t>(key);
        }
        
        return static_cast<std::size_t>(key);
    }

    inline const Node* Node::find_map_value(std::string_view key) const
    {
        const auto& kvps = storage_.get<detail::MapStorage>().kvps;

        for (const auto& [it_key, value] : kvps)
        {
            if (it_key == key)
                return value.get();
        }

        return nullptr;
    }

    inline Node* Node::find_map_value(std::string_view key)
    {
        return const_cast<Node*>(static_cast<const Node&>(*this).find_map_value(key));
    }
}
