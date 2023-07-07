#pragma once

/// @file userver/utils/meta_light.hpp
/// @brief Lightweight concepts
/// @see userver/utils/meta.hpp for more concepts

#include <type_traits>

#include <userver/utils/void_t.hpp>

USERVER_NAMESPACE_BEGIN

namespace meta {

namespace impl {

template <typename Default, typename AlwaysVoid,
          template <typename...> typename Trait, typename... Args>
struct Detector {
  using value_t = std::false_type;
  using type = Default;
};

template <typename Default, template <typename...> typename Trait,
          typename... Args>
struct Detector<Default, utils::void_t<Trait<Args...>>, Trait, Args...> {
  using value_t = std::true_type;
  using type = Trait<Args...>;
};

template <template <typename...> typename Template, typename T>
struct IsInstantiationOf : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct IsInstantiationOf<Template, Template<Args...>> : std::true_type {};

}  // namespace impl

/// @see utils::meta::kIsDetected
struct NotDetected {};

#if defined(__cpp_concepts) || defined(DOXYGEN)

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
/// if constexpr (utils::meta::kIsDetected<HasValueType, T>) { ... }
/// @endcode
template <template <typename...> typename Trait, typename... Args>
concept kIsDetected = requires { typename Trait<Args...>; };

#else

template <template <typename...> typename Trait, typename... Args>
inline constexpr bool kIsDetected =
    impl::Detector<NotDetected, void, Trait, Args...>::value_t::value;

#endif

/// @brief Produces the result type of a trait, or utils::meta::NotDetected if
/// it's incorrect for the given template args
/// @see utils::meta::kIsDetected
template <template <typename...> typename Trait, typename... Args>
using DetectedType =
    typename impl::Detector<NotDetected, void, Trait, Args...>::type;

/// @brief Produces the result type of a trait, or @a Default if it's incorrect
/// for the given template args
/// @see utils::meta::kIsDetected
template <typename Default, template <typename...> typename Trait,
          typename... Args>
using DetectedOr = typename impl::Detector<Default, void, Trait, Args...>::type;

/// Helps in definitions of traits for utils::meta::kIsDetected
template <typename T, typename U>
using ExpectSame = std::enable_if_t<std::is_same_v<T, U>>;

/// Returns `true` if the type if an instantiation of the specified template.
template <template <typename...> typename Template, typename T>
inline constexpr bool kIsInstantiationOf =
    impl::IsInstantiationOf<Template, T>::value;

/// Returns `true` if the type is a fundamental character type.
/// `signed char` and `unsigned char` are not character types.
template <typename T>
inline constexpr bool kIsCharacter =
    std::is_same_v<T, char> || std::is_same_v<T, wchar_t> ||
    std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>;

/// Returns `true` if the type is a true integer type (not `*char*` or `bool`)
/// `signed char` and `unsigned char` are integer types
template <typename T>
inline constexpr bool kIsInteger =
    std::is_integral_v<T> && !kIsCharacter<T> && !std::is_same_v<T, bool>;

}  // namespace meta

USERVER_NAMESPACE_END
