#pragma once

#include <userver/chaotic/convert/to.hpp>
#include <userver/utils/default_dict.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

template <typename T, typename U>
DefaultDict<T> Convert(const U& value, chaotic::convert::To<DefaultDict<T>>) {
    auto& extra = value.extra;
    auto dict = DefaultDict<T>{{extra.begin(), extra.end()}};

    if constexpr (meta::kIsOptional<decltype(value.__default__)>) {
        if (value.__default__) dict.SetDefault(*value.__default__);
    } else {
        dict.SetDefault(value.__default__);
    }

    return dict;
}

template <typename T, typename U>
U Convert(const DefaultDict<T>& value, chaotic::convert::To<U>) {
    U u;
    u.extra = {value.begin(), value.end()};
    if (value.HasDefaultValue()) u.__default__ = value.GetDefaultValue();
    return u;
}

}  // namespace utils

USERVER_NAMESPACE_END
