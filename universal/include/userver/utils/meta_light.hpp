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

#ifndef ARCADIA_ROOT

/// @brief Checks whether a trait is correct for the given template args
///
/// @deprecated Use a `requires` expression directly:
/// @code
/// if constexpr (requires { typename T::ValueType; }) { ... }
/// @endcode
template <template <typename...> typename Trait, typename... Args>
concept IsDetected = requires { typename Trait<Args...>; };

#endif

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

/// @brief Returns `true` if the type is an instantiation of the specified template.
/// @deprecated Use @ref meta::IsInstantiationOf instead.
template <template <typename...> typename Template, typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept kIsInstantiationOf = IsInstantiationOf<T, Template>;

/// @brief Returns `true` if the type (with remove cv-qualifiers) is an instantiation of the specified template.
/// @deprecated Use @ref meta::IsCvInstantiationOf instead.
template <template <typename...> typename Template, typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept kIsCvInstantiationOf = IsCvInstantiationOf<T, Template>;

/// @brief Returns `true` if the type is a true integer type (not `*char*` or `bool`)
/// `signed char` and `unsigned char` are integer types
/// @deprecated Use @ref meta::IsInteger instead.
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept kIsInteger = IsInteger<T>;

}  // namespace meta

USERVER_NAMESPACE_END
