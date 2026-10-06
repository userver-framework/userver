#pragma once

/// @file userver/utils/invoke_with_expected.hpp
/// @brief @copybrief utils::InvokeWithExpected

#include <concepts>
#include <exception>
#include <functional>
#include <type_traits>
#include <utility>

#include <userver/utils/expected.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Invoke a callable and return its result or exception in @ref utils::expected.
///
/// @param func Callable invoked once, preserving its value category.
/// @returns The decayed result, or a `std::exception_ptr` preserving the dynamic type
/// of a caught `std::exception`-derived exception. Supports void and move-only results.
/// Reference results are copied into the returned value; array and function references
/// decay to pointers and do not extend the lifetime of their referents.
/// @throws Non-`std::exception`-derived exceptions propagate; they are never stored.
template <std::invocable Func>
auto InvokeWithExpected(Func&& func) -> expected<std::decay_t<std::invoke_result_t<Func>>, std::exception_ptr> {
    try {
        if constexpr (std::is_void_v<std::invoke_result_t<Func>>) {
            std::invoke(std::forward<Func>(func));
            return {};
        } else {
            return std::invoke(std::forward<Func>(func));
        }
    } catch (const std::exception&) {
        return unexpected(std::current_exception());
    }
}

}  // namespace utils

USERVER_NAMESPACE_END
