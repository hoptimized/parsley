#pragma once

#include <type_traits>

namespace parsley
{
    template <typename A, typename B>
    constexpr bool is_same_v = std::is_same<A, B>{};
}
