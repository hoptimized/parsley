#pragma once

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace parsley
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////// Util ////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename T>
    std::optional<std::size_t> to_size_t(T&& value)
    {
        using U = std::remove_cvref_t<T>;

        if constexpr (std::is_integral_v<U>)
        {
            if constexpr (std::is_signed_v<U>)
            {
                if (value < 0)
                    return std::nullopt;
            }

            using Unsigned = std::make_unsigned_t<U>;
            if (static_cast<unsigned long long>(value) > std::numeric_limits<std::size_t>::max())
                return std::nullopt;

            return static_cast<std::size_t>(value);
        }
        else if constexpr (std::is_convertible_v<U, std::string_view>)
        {
            std::string_view s = value;

            std::size_t result{};
            auto [ptr, ec] = std::from_chars(
                s.data(),
                s.data() + s.size(),
                result
            );

            if (ec == std::errc{} && ptr == s.data() + s.size())
                return result;

            return std::nullopt;
        }
        else
        {
            return std::nullopt;
        }
    }

    // TODO: is this util really needed? Consider std::to_string?
    template <typename T>
    std::optional<std::string> to_string_key(const T& key)
    {
        using U = std::decay_t<T>;

        if constexpr (std::is_same_v<U, std::string>)
        {
            return key;
        }
        else if constexpr (std::is_same_v<U, std::string_view>)
        {
            return std::string(key);
        }
        else if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*>)
        {
            return std::string(key);
        }
        else
        {
            return std::nullopt;
        }
    }

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

        template <typename T>
        requires (!std::is_same_v<std::decay_t<T>, Node>)
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
            if (is_scalar())
                throw std::runtime_error("Cannot access scalar with subscript operator.");

            std::optional<size_t> idx = to_size_t(key);

            if (idx.has_value())
            {
                // Convert null node into a List
                if (is_null())
                {
                    storage_ = ListStorage{};
                    auto& values = std::get<ListStorage>(storage_).values;
                    values.emplace_back(std::make_unique<Node>());
                    return *values.back();
                }

                // Index within bounds, access the list
                if (is_list() && *idx < size())
                    return *std::get<ListStorage>(storage_).values[idx.value()];

                // Index is bounds+1, insert a new node
                if (is_list() && *idx == size())
                {
                    auto& values = std::get<ListStorage>(storage_).values;
                    values.emplace_back(std::make_unique<Node>());
                    return const_cast<Node&>(static_cast<const Node&>(*this)[idx.value()]);
                }

                // Access map by stringified key
                if (is_map())
                {
                    auto key = std::to_string(*idx);

                    if (auto* value = find_map_value(key))
                        return *value;

                    auto& kvps = std::get<MapStorage>(storage_).kvps;
                    kvps.emplace_back(key, std::make_unique<Node>());
                    return *kvps.back().second;
                }
            }
            else
            {
                // TODO: this is inefficient if we already got a string key
                auto str_key = to_string_key(key);
                if (!str_key.has_value())
                    throw std::runtime_error("Unable to convert key to string");

                // Convert null node into a Map
                if (is_null())
                {
                    storage_ = MapStorage{};
                    auto& kvps = std::get<MapStorage>(storage_).kvps;
                    kvps.emplace_back(std::make_pair(*str_key, std::make_unique<Node>()));
                    return const_cast<Node&>(static_cast<const Node&>(*this)[*str_key]);
                }

                // Convert list into Map
                if (is_list())
                {
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

                // Access by key or insert new element
                if (is_map())
                {
                    if (auto value = find_map_value(*str_key))
                    {
                        return *value;
                    }
                    else
                    {
                        auto& kvps = std::get<MapStorage>(storage_).kvps;
                        kvps.emplace_back(std::make_pair(*str_key, std::make_unique<Node>()));
                        return *kvps.back().second;
                    }
                }
            }

            throw std::runtime_error("Unhandled case.");
        }

        template <typename T>
        const Node& operator[](T key) const
        {
            if (is_null())
                throw std::runtime_error("Cannot access null node with subscript operator.");

            if (is_scalar())
                throw std::runtime_error("Cannot access scalar with subscript operator.");

            std::optional<size_t> idx = to_size_t(key);

            if (idx.has_value())
            {
                // Access list by index
                if (is_list())
                {
                    if (*idx < size())
                    {
                        return *std::get<ListStorage>(storage_).values[idx.value()];
                    }
                    else
                    {
                        throw std::runtime_error("Index out of bounds");
                    }
                }

                // Access map by stringified key
                if (is_map())
                {
                    auto key = std::to_string(*idx);

                    if (auto value = find_map_value(key))
                        return *value;

                    throw std::runtime_error("Key not found.");
                }
            }
            else
            {
                if (is_list())
                    throw std::runtime_error("Cannot access list with string key.");

                // TODO: this is inefficient if we already got a string key
                auto str_key = to_string_key(key);
                if (!str_key.has_value())
                    throw std::runtime_error("Unable to convert key to string.");

                // Access map by key
                if (is_map())
                {
                    if (auto value = find_map_value(*str_key))
                        return *value;

                    throw std::runtime_error("Key not found.");
                }
            }

            throw std::runtime_error("Unhandled case.");
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
