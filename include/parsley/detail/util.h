#pragma once

#include <memory>
#include <type_traits>

namespace parsley
{
    template <typename T>
    using decay_t = typename std::decay<T>::type;

    template <bool B, typename T = void>
    using enable_if_t = typename std::enable_if<B, T>::type;

    template <typename T>
    using remove_reference_t = typename std::remove_reference<T>::type;

    template <class T, class... Args>
    std::unique_ptr<T> make_unique(Args&&... args)
    {
        return std::unique_ptr<T>{ new T{ std::forward<Args>(args...)... }};
    }

    namespace detail
    {
        template <typename T>
        using enable_if_mutable_t = enable_if_t<!std::is_const<remove_reference_t<T>>::value>;

    }
}
