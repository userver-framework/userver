#pragma once

// Utilitary header for chaotic for a custom type serialization/parsing support

#include <type_traits>

#include <userver/chaotic/convert/to.hpp>
#include <userver/utils/default_dict.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

// Forwards `value.extra` and `value.__default__` fields onto DefaultDict<T>
template <typename T, typename U>
DefaultDict<T> Convert(U&& value, chaotic::convert::To<DefaultDict<T>>) {
    auto& extra = value.extra;

    using IteratorType = std::conditional_t<
        std::is_rvalue_reference_v<U&&>,
        decltype(std::move_iterator{extra.begin()}),
        decltype(extra.begin())>;
    auto dict = DefaultDict<T>{{IteratorType{extra.begin()}, IteratorType{extra.end()}}};

    if constexpr (meta::kIsOptional<decltype(value.__default__)>) {
        if (value.__default__) {
            dict.SetDefault(*std::forward<U>(value).__default__);
        }
    } else {
        dict.SetDefault(std::forward<U>(value).__default__);
    }

    return dict;
}

// Fills only `extra` and `__default__` fields of U
template <typename T, typename U>
U Convert(const DefaultDict<T>& value, chaotic::convert::To<U>) {
    U u;
    u.extra = {value.begin(), value.end()};
    if (value.HasDefaultValue()) {
        u.__default__ = value.GetDefaultValue();
    }
    return u;
}

}  // namespace utils

USERVER_NAMESPACE_END
