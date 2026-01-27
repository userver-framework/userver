#pragma once

/// @file userver/compiler/demangle.hpp
/// @brief @copybrief compiler::GetTypeName
/// @ingroup userver_universal

#include <chrono>
#include <string>
#include <typeindex>

USERVER_NAMESPACE_BEGIN

/// Compiler and C++ language related tweaks
namespace compiler {

/// Returns a human-readable representation of provided type name.
std::string GetTypeName(std::type_index type);

namespace detail {

template <typename T>
constexpr std::string_view GetTypeAlias() {
    if constexpr (std::is_same_v<T, std::string>) {
        return "std::string";
    } else if constexpr (std::is_same_v<T, std::string_view>) {
        return "std::string_view";
    } else if constexpr (std::is_same_v<T, std::chrono::nanoseconds>) {
        return "std::chrono::nanoseconds";
    } else if constexpr (std::is_same_v<T, std::chrono::microseconds>) {
        return "std::chrono::microseconds";
    } else if constexpr (std::is_same_v<T, std::chrono::milliseconds>) {
        return "std::chrono::milliseconds";
    } else if constexpr (std::is_same_v<T, std::chrono::seconds>) {
        return "std::chrono::seconds";
    } else if constexpr (std::is_same_v<T, std::chrono::minutes>) {
        return "std::chrono::minutes";
    } else if constexpr (std::is_same_v<T, std::chrono::hours>) {
        return "std::chrono::hours";
    } else if constexpr (std::is_same_v<T, std::chrono::steady_clock::time_point>) {
        return "std::chrono::steady_clock::time_point";
    } else if constexpr (std::is_same_v<T, std::chrono::system_clock::time_point>) {
        return "std::chrono::system_clock::time_point";
    } else {
        return {};
    }
}

}  // namespace detail

#if defined(__GNUC__) && !defined(DOXYGEN)

namespace detail {

template <typename T>
constexpr std::string_view GetFullTypeName() {
    constexpr auto alias = GetTypeAlias<T>();
    if constexpr (!alias.empty()) {
        return alias;
    } else {
        constexpr std::string_view name = __PRETTY_FUNCTION__;
        constexpr auto begin_pos = name.find("T = ");
        constexpr auto end_pos = name.find_first_of(";]", begin_pos);
        return name.substr(begin_pos + 4, end_pos - begin_pos - 4);
    }
}

}  // namespace detail

template <typename T>
constexpr std::string_view GetTypeName() {
    return detail::GetFullTypeName<typename std::decay_t<T>>();
}

#else  // !__GNUC__

/// @brief Returns a human-readable representation of provided type name.
///
/// GetTypeName(typeidT)) outputs the type, not the alias. For std::chrono
/// functions it gives unreadable results:
///     std::chrono::duration<long, std::ratio<1l, 1l> > - it's `seconds`
///
/// The `GetTypeName<T>()` provides a more readable output.
template <typename T>
std::string_view GetTypeName() {
    constexpr auto alias = detail::GetTypeAlias<typename std::decay_t<T>>();
    if constexpr (!alias.empty()) {
        return alias;
    } else {
        static const auto name = GetTypeName(typeid(T));
        return name;
    }
}

#endif  // __GNUC__

}  // namespace compiler

USERVER_NAMESPACE_END
