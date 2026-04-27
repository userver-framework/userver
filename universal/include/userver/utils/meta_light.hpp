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

template <typename Default, template <typename...> typename Trait, typename... Args>
struct Detector {
    using type = Default;
};

template <typename Default, template <typename...> typename Trait, typename... Args>
requires requires { typename Trait<Args...>; }
struct Detector<Default, Trait, Args...> {
    using type = Trait<Args...>;
};

template <template <typename...> typename Template, typename T>
struct IsInstantiationOf : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct IsInstantiationOf<Template, Template<Args...>> : std::true_type {};

}  // namespace impl

/// @see utils::meta::IsDetected
struct NotDetected {};

/// @brief Checks whether a trait is correct for the given template args
///
/// Implements the pre-cpp20-concepts detection idiom.
///
/// To use, define a templated type alias (a "trait"), which for some type
/// either is correct and produces ("detects") some result type,
/// or is SFINAE-d out. Example:
///
/// @code
/// template <typename T>
/// using HasValueType = typename T::ValueType;
/// ...
/// if constexpr (utils::meta::IsDetected<HasValueType, T>) { ... }
/// @endcode
///
/// @deprecated Prefer using `requires` expressions directly in new code:
/// @code
/// if constexpr (requires { typename T::ValueType; }) { ... }
/// @endcode
template <template <typename...> typename Trait, typename... Args>
concept IsDetected = requires { typename Trait<Args...>; };

/// @brief Produces the result type of a trait, or utils::meta::NotDetected if
/// it's incorrect for the given template args
/// @see utils::meta::IsDetected
template <template <typename...> typename Trait, typename... Args>
using DetectedType = typename impl::Detector<NotDetected, Trait, Args...>::type;

/// @brief Produces the result type of a trait, or @a Default if it's incorrect
/// for the given template args
/// @see utils::meta::IsDetected
template <typename Default, template <typename...> typename Trait, typename... Args>
using DetectedOr = typename impl::Detector<Default, Trait, Args...>::type;

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
