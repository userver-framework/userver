#pragma once

/// @file userver/chaotic/convert.hpp
/// @brief @copybrief chaotic::ConvertTo

#include <cstdint>
#include <string_view>
#include <type_traits>

#include <userver/chaotic/convert/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {
namespace convert {

namespace impl {

template <typename YourType>
constexpr void ReportMissingConvert(std::string_view, chaotic::convert::To<YourType>) {
    static_assert(
        sizeof(YourType) && false,
        "There is no `YourType Convert(std::string_view, chaotic::convert::To<YourType>)` in namespace of `YourType` "
        "or `chaotic::convert`. Probably you have not provided a `Convert` function overload."
    );
}

template <typename YourType>
constexpr void ReportMissingConvert(std::int64_t, chaotic::convert::To<YourType>) {
    static_assert(
        sizeof(YourType) && false,
        "There is no `YourType Convert(std::int64_t, chaotic::convert::To<YourType>)` in namespace of `YourType` or "
        "`chaotic::convert`. Probably you have not provided a `Convert` function overload."
    );
}

template <class T, class YourType>
constexpr void ReportMissingConvert(T&&, chaotic::convert::To<YourType>) {
    static_assert(
        sizeof(YourType) && false,
        "There is no `YourType Convert(T&&, chaotic::convert::To<YourType>)` in namespace of `T`, `YourType` or "
        "`chaotic::convert`. Probably you have not provided a `Convert` function overload."
    );
}

template <typename YourType, typename T>
constexpr void ReportMissingConvert(const YourType&, chaotic::convert::To<T>) {
    static_assert(
        sizeof(YourType) && false,
        "There is no `T Convert(const YourType&, chaotic::convert::To<T>)` in namespace of `T`, `YourType` or "
        "`chaotic::convert`. Probably you have not provided a `Convert` function overload."
    );
}

template <typename TargetType>
class ConvertCPO {
public:
    template <typename... Args>
    constexpr void operator()(Args&&... args) const noexcept {
        static_assert(sizeof...(Args) == 1, "Should be used with a single argument only");
        impl::ReportMissingConvert(std::forward<Args>(args)..., chaotic::convert::To<TargetType>{});
    }

    template <typename T>
    constexpr auto operator()(T&& value
    ) const -> decltype(Convert(std::forward<T>(value), chaotic::convert::To<TargetType>{}))
    {
        return Convert(std::forward<T>(value), chaotic::convert::To<TargetType>{});
    }
};

}  // namespace impl

template <typename T, typename U>
constexpr U Convert(const T& value, To<U>)
requires std::is_constructible_v<U, const T&>
{
    return U{value};
}

}  // namespace convert

/// @brief Helper function to dispatch to user defined `Convert(X, To<Y>)` functions.
/// Use like `chaotic::ConvertTo<TargetType>(value_from)`.
template <typename To>
constexpr inline convert::impl::ConvertCPO<To> ConvertTo{};

}  // namespace chaotic

USERVER_NAMESPACE_END
