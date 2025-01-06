#pragma once

#include <type_traits>

#include <userver/chaotic/convert/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

template <typename T, typename U>
U Convert(const T& value, To<U>) {
    static_assert(
        std::is_constructible_v<U, const T&>,
        "There is no `Convert(const Value&, chaotic::convert::To<T>)` in "
        "namespace of `T` or `chaotic::convert`. Probably you have not provided "
        "a `Convert` function overload."
    );

    return U{value};
}

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
