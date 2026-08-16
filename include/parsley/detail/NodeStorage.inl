#pragma once

// This include helps clangd understand the code
#include "parsley/Node.h"

#include "parsley/NodeType.h"

#include <stdexcept>

namespace parsley
{
    namespace detail
    {
        // Compile-time mapping from storage type to NodeType enum value
        template <typename T> struct NodeTypeOf;
        template <> struct NodeTypeOf<NullStorage> { static constexpr NodeType value = NodeType::Null; };
        template <> struct NodeTypeOf<ScalarStorage> { static constexpr NodeType value = NodeType::Scalar; };
        template <> struct NodeTypeOf<ListStorage> { static constexpr NodeType value = NodeType::List; };
        template <> struct NodeTypeOf<MapStorage> { static constexpr NodeType value = NodeType::Map; };

        inline ListStorage::~ListStorage() = default;
        inline ListStorage::ListStorage(ListStorage&&) noexcept = default;
        inline ListStorage& ListStorage::operator=(ListStorage&&) noexcept = default;

        inline MapStorage::~MapStorage() = default;
        inline MapStorage::MapStorage(MapStorage&&) noexcept = default;
        inline MapStorage& MapStorage::operator=(MapStorage&&) noexcept = default;

        inline NodeStorage::NodeStorage() : type_(NodeType::Null)
        {
            new (&null_) NullStorage();
        }

        inline NodeStorage::NodeStorage(NullStorage v) : type_(NodeType::Null)
        {
            new (&null_) NullStorage(std::move(v));
        }

        inline NodeStorage::NodeStorage(ScalarStorage v) : type_(NodeType::Scalar)
        {
            new (&scalar_) ScalarStorage(std::move(v));
        }

        inline NodeStorage::NodeStorage(ListStorage v) : type_(NodeType::List)
        {
            new (&list_) ListStorage(std::move(v));
        }

        inline NodeStorage::NodeStorage(MapStorage v) : type_(NodeType::Map)
        {
            new (&map_) MapStorage(std::move(v));
        }

        inline NodeStorage::NodeStorage(NodeStorage&& other) noexcept : type_(other.type_)
        {
            move(std::move(other));
        }

        inline NodeStorage& NodeStorage::operator=(NodeStorage&& other) noexcept
        {
            if (this != &other)
            {
                destroy();
                type_ = other.type_;
                move(std::move(other));
            }
            return *this;
        }

        inline NodeStorage::~NodeStorage()
        {
            destroy();
        }

        inline NodeType NodeStorage::type() const
        {
            return type_;
        }

        template <typename T>
        inline bool NodeStorage::is() const
        {
            return type_ == NodeTypeOf<T>::value;
        }

        inline bool NodeStorage::is(NodeType type) const
        {
            return type_ == type;
        }

        inline bool NodeStorage::operator==(const NodeStorage& other) const
        {
            if (this == &other)
                return true;

            if (type_ != other.type_)
                return false;

            switch (type_)
            {
                case NodeType::Null:
                    return true;
                case NodeType::Scalar:
                    return scalar_.value == other.scalar_.value;
                case NodeType::List:
                    return list_.values == other.list_.values;
                case NodeType::Map:
                    return map_.kvps == other.map_.kvps;                
            }

            throw std::runtime_error("Unhandled node type.");
        }

        template <typename T>
        inline T& NodeStorage::get()
        {
            if (!is<T>())
                throw std::runtime_error("Wrong node type.");

            return get_internal(static_cast<T*>(nullptr));
        }

        template <typename T>
        inline const T& NodeStorage::get() const
        {
            return const_cast<NodeStorage*>(this)->get<T>();
        }

        template <typename T>
        inline T* NodeStorage::try_get()
        {
            if (!is<T>())
                return nullptr;

            return &get_internal(static_cast<T*>(nullptr));
        }

        template <typename T>
        inline const T* NodeStorage::try_get() const
        {
            return const_cast<NodeStorage*>(this)->try_get<T>();
        }

        inline void NodeStorage::move(NodeStorage&& other)
        {
            switch (type_)
            {
                case NodeType::Null:
                    new (&null_) NullStorage(std::move(other.null_));
                    break;
                case NodeType::Scalar:
                    new (&scalar_) ScalarStorage(std::move(other.scalar_));
                    break;
                case NodeType::List:
                    new (&list_) ListStorage(std::move(other.list_));   
                    break;
                case NodeType::Map:
                    new (&map_) MapStorage(std::move(other.map_));    
                    break;
            }
        }

        inline void NodeStorage::destroy()
        {
            switch (type_)
            {
                case NodeType::Null:
                    null_.~NullStorage();
                    break;
                case NodeType::Scalar:
                    scalar_.~ScalarStorage();
                    break;
                case NodeType::List:
                    list_.~ListStorage();
                    break;
                case NodeType::Map:
                    map_.~MapStorage();
                    break;
            }
        }
    }
}
