#pragma once

#include "parsley/core/StringView.h"
#include "parsley/detail/util.h"
#include "parsley/NodeType.h"

#include <memory>
#include <string>
#include <type_traits>
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
        //-------------------------------------------------------------------------------------------------
        // Nested Types

        template <typename NodeRef>
        class EntryBase;

        using Entry = EntryBase<Node&>;
        using ConstEntry = EntryBase<const Node&>;

        template<typename ListIter, typename MapIter, typename EntryT> 
        class IteratorBase;

        using Iterator = IteratorBase<
            std::vector<std::unique_ptr<Node>>::iterator,
            std::vector<std::pair<std::string, std::unique_ptr<Node>>>::iterator,
            Entry>;

        using ConstIterator = IteratorBase<
            std::vector<std::unique_ptr<Node>>::const_iterator,
            std::vector<std::pair<std::string, std::unique_ptr<Node>>>::const_iterator,
            ConstEntry>;
            
        //-------------------------------------------------------------------------------------------------
        // Construction & Assignment

        Node();
        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;
        Node(Node&&);
        Node& operator=(Node&&);

        template <typename T, typename = enable_if_t<!std::is_same<decay_t<T>, Node>::value>>
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

        Iterator begin();
        Iterator end();

        ConstIterator begin() const;
        ConstIterator cbegin() const;
        ConstIterator end() const;        
        ConstIterator cend() const;

        //-------------------------------------------------------------------------------------------------
        // Identity

        NodeType type() const;

        bool is_null() const;
        bool is_scalar() const;
        bool is_list() const;
        bool is_map() const;
        bool is_collection() const;

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

    template <typename NodeRef>
    struct Node::EntryBase
    {
        const StringView key;
        NodeRef value;
        
        EntryBase(StringView key, NodeRef value);

        // Converting constructor: lets Entry convert to ConstEntry
        template<
            typename OtherNodeRef,
            typename = enable_if_t<std::is_convertible<OtherNodeRef, NodeRef>::value>>
        EntryBase(const EntryBase<OtherNodeRef>& other);

        operator NodeRef();
        template <typename T> T as() const;
        
        template <typename T, typename U = NodeRef, typename = detail::enable_if_mutable_t<U>>
        EntryBase& operator=(T&& val);

        // TODO: add missing Node methods
    };

    template<typename ListIter, typename MapIter, typename EntryT>
    class Node::IteratorBase
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Entry;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = Entry;
#if defined(__cpp_lib_ranges)
        using iterator_concept = std::forward_iterator_tag;
#endif

        IteratorBase(NodeType type, ListIter list_it, MapIter map_it);

        // Converting constructor: lets Iterator convert to ConstIterator
        template<
            typename OtherListIter,
            typename OtherMapIter, 
            typename OtherEntryT,
            typename = enable_if_t<
                std::is_convertible<OtherListIter, ListIter>::value &&
                std::is_convertible<OtherMapIter, MapIter>::value &&
                std::is_convertible<OtherEntryT, EntryT>::value>>
        IteratorBase(const IteratorBase<OtherListIter, OtherMapIter, OtherEntryT>& other);

        EntryT operator*() const;

        struct ArrowProxy
        {
            EntryT entry;
            EntryT* operator->() { return &entry; }
        };
        ArrowProxy operator->() const { return ArrowProxy{**this}; }

        IteratorBase& operator++();
        IteratorBase operator++(int);

        template<typename OtherListIter, typename OtherMapIter, typename OtherEntryT>
        bool operator==(const IteratorBase<OtherListIter, OtherMapIter, OtherEntryT>& other) const;

        template<typename OtherListIter, typename OtherMapIter, typename OtherEntryT>
        bool operator!=(const IteratorBase<OtherListIter, OtherMapIter, OtherEntryT>& other) const;

    private:
        NodeType type_;
        ListIter list_it_;
        MapIter map_it_;

        // Give conversion constructor access to internals
        template<typename, typename, typename> friend class Node::IteratorBase;
    };
}

#include "parsley/detail/Node.inl"
#include "parsley/detail/NodeEntry.inl"
#include "parsley/detail/NodeIterator.inl"
#include "parsley/detail/NodeStorage.inl"
