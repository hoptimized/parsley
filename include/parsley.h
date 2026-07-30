#pragma once

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace parsley
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////// Standard Compatibility //////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename T>
    using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////// Forward Declarations ////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    class Node;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////// Storage /////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    struct NullStorage
    {
    };

    struct ScalarStorage
    {
        std::string value;
    };

    struct ListStorage
    {
        std::vector<std::unique_ptr<Node>> values;
    };

    struct MapStorage
    {
        std::vector<std::pair<std::string, std::unique_ptr<Node>>> kvps;
    };

    using NodeStorage = std::variant<NullStorage, ScalarStorage, ListStorage, MapStorage>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////// Transfer (Base Template) ////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename T, typename = void>
    struct Transfer
    {
        static void read(const Node& node, T& val);
        static void write(Node& node, const T& val);
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////// Node ////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    
    class Node
    {
    public:
        Node() = default;
        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;
        Node(Node&&) = default;
        Node& operator=(Node&&) = default;

        //-------------------------------------------------------------------------------------------------
        // Construction & Assignment

        template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Node>>>
        Node(T&& val)
        {
            Transfer<std::decay_t<T>>::write(*this, std::forward<T>(val));
        }

        template <typename T>
        Node& operator=(T&& val)
        {
            Transfer<std::decay_t<T>>::write(*this, std::forward<T>(val));
            return *this;
        }

        //-------------------------------------------------------------------------------------------------
        // Modifiers

        template <typename T>
        void push_back(T&& val)
        {
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

            auto& values = std::get<ListStorage>(storage_).values;
            values.emplace_back(std::move(child));
        }

        void clear()
        {
            storage_ = NullStorage{};
        }

        //-------------------------------------------------------------------------------------------------
        // Access

        template <typename T>
        T as() const
        {
            T val{};
            Transfer<T>::read(*this, val);
            return val;
        }

        template <typename T>
        Node& operator[](T key) 
        {
            return *get_collection_node(key, /*allow_insert=*/true);
        }

        template <typename T>
        const Node& operator[](T key) const 
        {
            return *const_cast<Node*>(this)->get_collection_node(key, /*allow_insert=*/false);
        }

        //-------------------------------------------------------------------------------------------------
        // Identity

        bool is_null() const { return std::holds_alternative<NullStorage>(storage_); }
        bool is_scalar() const { return std::holds_alternative<ScalarStorage>(storage_); }
        bool is_list() const { return std::holds_alternative<ListStorage>(storage_); }
        bool is_map() const { return std::holds_alternative<MapStorage>(storage_); }

        //-------------------------------------------------------------------------------------------------
        // Capacity

        bool empty() const
        {
            return size() == 0;
        }

        std::size_t size() const
        {
            if (const auto* null = std::get_if<NullStorage>(&storage_))
                return 0;
            if (const auto* scalar = std::get_if<ScalarStorage>(&storage_))
                return 1;
            if (const auto* list = std::get_if<ListStorage>(&storage_))
                return list->values.size();
            if (const auto* map = std::get_if<MapStorage>(&storage_))
                return map->kvps.size();
            return 0;
        }

        //-------------------------------------------------------------------------------------------------
        // Low-level access

        void set_scalar(std::string s)
        {
            storage_ = ScalarStorage{ std::move(s) };
        }

        const std::string& get_scalar() const
        {
            const auto* s = std::get_if<ScalarStorage>(&storage_);
            if (!s)
                throw std::runtime_error("Node does not hold a scalar");
            return s->value;
        }

    private:
        template <typename T>
        Node* get_collection_node(T key, bool allow_insert)
        {
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

                auto& values = std::get<ListStorage>(storage_).values;

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

                    auto old_values = std::move(std::get<ListStorage>(storage_).values);

                    storage_ = MapStorage{};
                    auto& kvps = std::get<MapStorage>(storage_).kvps;

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
                        auto& kvps = std::get<MapStorage>(storage_).kvps;
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
        static size_t to_index(T key)
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

        const Node* find_map_value(std::string_view key) const
        {
            const auto& kvps = std::get<MapStorage>(storage_).kvps;

            for (const auto& [it_key, value] : kvps)
            {
                if (it_key == key)
                    return value.get();
            }

            return nullptr;
        }

        Node* find_map_value(std::string_view key)
        {
            return const_cast<Node*>(static_cast<const Node&>(*this).find_map_value(key));
        }

        NodeStorage storage_ = NullStorage{};
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////// Transfer (Implementation) ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    //-----------------------------------------------------------------------------------------------------
    // Transfer: fallback

    template <typename T, typename E>
    void Transfer<T, E>::write(Node& node, const T& val)
    {
        std::ostringstream oss;
        oss << val;
        node.set_scalar(oss.str());
    }

    template <typename T, typename E>
    void Transfer<T, E>::read(const Node& node, T& val)
    {
        std::istringstream iss(node.get_scalar());
        iss >> val;
    }

    //-----------------------------------------------------------------------------------------------------
    // Transfer: std::vector
    
    template <typename T, typename A>
    struct Transfer<std::vector<T, A>>
    {
        static void write(Node& node, const std::vector<T, A>& val)
        {
            node.clear();
            for (const auto& elem : val)
                node.push_back(elem);
        }

        static void read(const Node& node, std::vector<T, A>& val)
        {
            val.clear();
            val.reserve(node.size());
            for (std::size_t i = 0; i < node.size(); ++i)
                val.push_back(node[i].as<T>());
        }
    };
}
