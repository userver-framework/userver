#pragma once

/// @file userver/utils/meta_light.hpp
/// @brief Lightweight concepts
/// @see userver/utils/meta.hpp for more concepts
/// @ingroup userver_universal

// Don't add new includes here! Put concepts that require them in meta.hpp.
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace meta {

namespace impl {

template <template <typename...> typename Template, typename T>
struct IsInstantiationOf : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct IsInstantiationOf<Template, Template<Args...>> : std::true_type {};

}  // namespace impl

/// @brief Returns `true` if the type is an instantiation of the specified template.
template <typename T, template <typename...> typename Template>
concept IsInstantiationOf = impl::IsInstantiationOf<Template, T>::value;

/// @brief Returns `true` if the type (with remove cv-qualifiers) is an instantiation of the specified template.
template <typename T, template <typename...> typename Template>
concept IsCvInstantiationOf = IsInstantiationOf<std::remove_cv_t<T>, Template>;

/// Returns `true` if the type is a fundamental character type.
/// `signed char` and `unsigned char` are not character types.
template <typename T>
concept IsCharacter =
    std::is_same_v<T, char> || std::is_same_v<T, wchar_t> || std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>;

/// Returns `true` if the type is a true integer type (not `*char*` or `bool`)
/// `signed char` and `unsigned char` are integer types
template <typename T>
concept IsInteger = std::is_integral_v<T> && !IsCharacter<T> && !std::is_same_v<T, bool>;

}  // namespace meta

USERVER_NAMESPACE_END
