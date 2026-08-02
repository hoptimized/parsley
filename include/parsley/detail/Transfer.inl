#pragma once

// This include helps clangd understand the code
#include "parsley/Transfer.h"

#include "parsley/Node.h"
#include "parsley/core/StringView.h"

#include <sstream>
#include <vector>

namespace parsley
{
    //-----------------------------------------------------------------------------------------------------
    // Streaming fallback

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
    // bool

    template <>
    struct Transfer<bool, void>
    {
        static void write(Node& node, const bool& val)
        {
            node.set_scalar(val ? "true" : "false");
        }

        static void read(const Node& node, bool& val)
        {
            auto& str = node.get_scalar();
            
            if (str == "true" || str == "on" || str == "1")
            {
                val = true;
            }
            else if (str == "false" || str == "off" || str == "0")
            {
                val = false;
            }
            else
            {
                throw std::runtime_error("Cannot cast to bool: \"" + str + "\"");
            }
        }
    };

    //-----------------------------------------------------------------------------------------------------
    // StringView

    template <>
    struct Transfer<StringView, void>
    {
        static void write(Node& node, const StringView& val)
        {
            node.set_scalar(val);
        }

        static void read(const Node& node, StringView& val)
        {
            val = node.get_scalar();
        }
    };

    //-----------------------------------------------------------------------------------------------------
    // std::vector
    
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
