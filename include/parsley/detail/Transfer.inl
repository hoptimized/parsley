#pragma once

// This include helps clangd understand the code
#include "parsley/Transfer.h"

#include "parsley/Node.h"
#include "parsley/core/StringView.h"
#include "parsley/core/StringViewInputBuffer.h"
#include "parsley/detail/util.h"

#include <limits>
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
        StringViewInputBuffer buf{ node.get_scalar() };
        std::istream is(&buf);
        is >> val;
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
            const StringView sv = node.get_scalar();
            
            if (sv == "true" || sv == "on" || sv == "1")
            {
                val = true;
            }
            else if (sv == "false" || sv == "off" || sv == "0")
            {
                val = false;
            }
            else
            {
                throw std::runtime_error("Cannot cast to bool: \"" + sv.to_owned() + "\"");
            }
        }
    };

    //-----------------------------------------------------------------------------------------------------
    // Narrow integer types (uint8_t, int8_t, and anything that can be interpreted as a char).
    // Stream them through a wider type so they're treated as numbers, not characters.

    template <typename T>
    struct Transfer<T, enable_if_t<
        std::is_integral<T>::value &&
        (sizeof(T) == 1) &&
        !std::is_same<T, bool>::value
    >>
    {
        using wide_t = typename std::conditional<std::is_signed<T>::value, int, unsigned int>::type;

        static void write(Node& node, const T& val)
        {
            std::ostringstream oss;
            oss << static_cast<wide_t>(val);
            node.set_scalar(oss.str());
        }

        static void read(const Node& node, T& val)
        {
            StringViewInputBuffer buf{ node.get_scalar() };
            std::istream is(&buf);

            wide_t tmp;
            is >> tmp;

            if (is.fail())
            {
                throw std::runtime_error("Cannot cast to integer: \"" + node.get_scalar().to_owned() + "\"");
            }

            if (tmp < static_cast<wide_t>(std::numeric_limits<T>::min()) ||
                tmp > static_cast<wide_t>(std::numeric_limits<T>::max()))
            {
                throw std::runtime_error("Value out of range for target type: \"" + node.get_scalar().to_owned() + "\"");
            }
            
            val = static_cast<T>(tmp);
        }
    };

    //-----------------------------------------------------------------------------------------------------
    // String-like

    template <>
    struct Transfer<std::string>
    {
        static void write(Node& node, const std::string& val)
        {
            node.set_scalar(val);
        }

        static void write(Node& node, std::string&& val)
        {
            node.set_scalar(std::move(val));
        }

        static void read(const Node& node, std::string& val)
        {
            val = node.get_scalar().to_owned();
        }
    };

    template <>
    struct Transfer<StringView>
    {
        static void write(Node& node, const StringView& val)
        {
            node.set_scalar(val.to_owned());
        }

        // read() rebinds into node-owned storage (no copy). Safe as long as the 
        // Node outlives the StringView, same lifetime contract as get_scalar().
        static void read(const Node& node, StringView& val)
        {
            val = node.get_scalar();
        }
    };

    // Anything convertible to StringView, for write-only purposes (literals, const char*, 
    // string_view-like types, etc).
    template <typename T>
    struct Transfer<T, enable_if_t<std::is_convertible<T, StringView>::value>>
    {
        static void write(Node& node, const T& val)
        {
            node.set_scalar(StringView(val).to_owned());
        }

        // No read() - these types have no defined way to own a copy of scalar data, 
        // so reading into them isn't offered.
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
