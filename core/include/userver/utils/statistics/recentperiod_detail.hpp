#pragma once

#include <userver/utils/void_t.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::detail {

// Detect if the result type provides Add(Counter, Duration, Duration) function
template <typename Result, typename Counter, typename Duration,
          typename = void_t<>>
struct ResultWantsAddFunction : std::false_type {};

template <typename Result, typename Counter, typename Duration>
struct ResultWantsAddFunction<
    Result, Counter, Duration,
    void_t<decltype(std::declval<Result>().Add(
        std::declval<Counter>(), std::declval<Duration>(),
        std::declval<Duration>()))>> : std::true_type {};

template <typename Result, typename Counter, typename Duration>
inline constexpr bool kResultWantsAddFunction =
    ResultWantsAddFunction<Result, Counter, Duration>::value;

// Detect if a counter can be added to the result
template <typename Result, typename Counter, typename = void_t<>>
struct ResultCanUseAddAssign : std::false_type {};

template <typename Result, typename Counter>
struct ResultCanUseAddAssign<
    Result, Counter,
    void_t<decltype(std::declval<Result&>() += std::declval<const Counter&>())>>
    : std::true_type {};

template <typename Result, typename Counter>
inline constexpr bool kResultCanUseAddAssign =
    ResultCanUseAddAssign<Result, Counter>::value;

// Detect if a Counter provides a Reset function
template <typename Counter, typename = void_t<>>
struct CanReset : std::false_type {};

template <typename Counter>
struct CanReset<Counter, void_t<decltype(std::declval<Counter&>().Reset())>>
    : std::true_type {};

template <typename Counter>
inline constexpr bool kCanReset = CanReset<Counter>::value;

}  // namespace utils::statistics::detail

USERVER_NAMESPACE_END
