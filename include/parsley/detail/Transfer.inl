#pragma once

// This include helps clangd understand the code
#include "parsley/Transfer.h"

#include "parsley/Node.h"

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
