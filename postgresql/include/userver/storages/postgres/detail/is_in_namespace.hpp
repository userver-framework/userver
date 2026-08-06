#pragma once

#include <string_view>
#include <type_traits>

#include <boost/current_function.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

#ifdef __clang__
constexpr std::string_view kExpectedPrefix = "postgres::detail::IsInNamespaceImpl(std::string_view) [T = ";
#else
constexpr std::string_view kExpectedPrefix = "postgres::detail::IsInNamespaceImpl(std::string_view) [with T = ";
#endif

template <typename T>
constexpr bool IsInNamespaceImpl(std::string_view nsp) {
    constexpr std::string_view fname = BOOST_CURRENT_FUNCTION;
    constexpr auto pos = fname.find(kExpectedPrefix);
    if (pos == std::string_view::npos) {
        return false;
    }
    constexpr std::string_view fname_short{fname.data() + pos, fname.size() - pos};
    static_assert(!fname_short.empty(), "Your compiler produces an unexpected function pretty name");
    return fname_short.substr(kExpectedPrefix.size()).starts_with(nsp) &&
           fname_short.substr(kExpectedPrefix.size() + nsp.size()).starts_with("::");
}

template <typename T>
concept IsInStdNamespace = detail::IsInNamespaceImpl<std::remove_cvref_t<T>>("std");
template <typename T>
concept IsInBoostNamespace = detail::IsInNamespaceImpl<std::remove_cvref_t<T>>("boost");

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
