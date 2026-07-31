#pragma once

#include "parsley/core/StringView.h"
#include "parsley/detail/util.h"
#include "parsley/NodeType.h"

#include <memory>
#include <string>
#include <vector>

namespace parsley
{
    class Node;

    namespace detail
    {
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

            ListStorage() = default;
            ~ListStorage();

            ListStorage(ListStorage&&) noexcept;
            ListStorage& operator=(ListStorage&&) noexcept;

            ListStorage(const ListStorage&) = delete;
            ListStorage& operator=(const ListStorage&) = delete;
        };

        struct MapStorage
        {
            std::vector<std::pair<std::string, std::unique_ptr<Node>>> kvps;

            MapStorage() = default;
            ~MapStorage();

            MapStorage(MapStorage&&) noexcept;
            MapStorage& operator=(MapStorage&&) noexcept;

            MapStorage(const MapStorage&) = delete;
            MapStorage& operator=(const MapStorage&) = delete;
        };

        class NodeStorage
        {
        public:
            NodeStorage();

            NodeStorage(NullStorage v);
            NodeStorage(ScalarStorage v);
            NodeStorage(ListStorage v);
            NodeStorage(MapStorage v);

            NodeStorage(NodeStorage&& other) noexcept;
            NodeStorage& operator=(NodeStorage&& other) noexcept;
            
            NodeStorage(const NodeStorage&) = delete;
            NodeStorage& operator=(const NodeStorage&) = delete;

            ~NodeStorage();

            NodeType type() const;
            
            bool is(NodeType type) const;
            template <typename T> bool is() const;

            template <typename T> T& get();
            template <typename T> const T& get() const;
            template <typename T> T* try_get();
            template <typename T> const T* try_get() const;

        private:
            void move(NodeStorage&& other);
            void destroy();

            NullStorage& get_internal(NullStorage*) { return null_; }
            ScalarStorage& get_internal(ScalarStorage*) { return scalar_; }
            ListStorage& get_internal(ListStorage*) { return list_; }
            MapStorage& get_internal(MapStorage*) { return map_; }

            NodeType type_;

            union
            {
                NullStorage null_;
                ScalarStorage scalar_;
                ListStorage list_;
                MapStorage map_;
            };
        };
    }

    class Node
    {
    public:
        Node();
        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;
        Node(Node&&);
        Node& operator=(Node&&);

        //-------------------------------------------------------------------------------------------------
        // Construction & Assignment

        template <typename T, typename = std::enable_if_t<!is_same_v<std::decay_t<T>, Node>>>
        Node(T&& val);

        template <typename T> Node& operator=(T&& val);

        //-------------------------------------------------------------------------------------------------
        // Modifiers

        template <typename T> void push_back(T&& val);

        void clear();

        //-------------------------------------------------------------------------------------------------
        // Access

        template <typename T> T as() const;

        template <typename T> Node& operator[](T key);
        template <typename T> const Node& operator[](T key) const;

        //-------------------------------------------------------------------------------------------------
        // Identity

        NodeType type() const;

        bool is_null() const;
        bool is_scalar() const;
        bool is_list() const;
        bool is_map() const;

        bool is(NodeType type) const;

        //-------------------------------------------------------------------------------------------------
        // Capacity

        bool empty() const;
        std::size_t size() const;

        //-------------------------------------------------------------------------------------------------
        // Low-level access

        void set_scalar(std::string s);
        const std::string& get_scalar() const;

    private:
        Node* get_collection_node(size_t idx, bool allow_insert);
        Node* get_collection_node(StringView key, bool allow_insert);

        const Node* find_map_value(StringView key) const;
        Node* find_map_value(StringView key);

        detail::NodeStorage storage_ = detail::NullStorage{};
    };
}

#include "parsley/detail/Node.inl"
#include "parsley/detail/NodeStorage.inl"
