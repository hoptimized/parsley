#pragma once

#include <type_traits>

namespace parsley
{
    namespace detail
    {
        template <typename A, typename B>
        constexpr bool is_same_v = std::is_same<A, B>{};

        template <typename T>
        using enable_if_mutable_t = std::enable_if_t<!std::is_const<std::remove_reference_t<T>>::value>;
    }
}
