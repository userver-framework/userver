#pragma once
#include <type_traits>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

template <typename... T>
struct TypeList {};


template <typename... Ts>
consteval auto Size(TypeList<Ts...>) {
  return sizeof...(Ts);
}

template <template <typename...> typename Check, typename... Ts>
consteval auto AnyOf(TypeList<Ts...>) {
  return (Check<Ts>::value || ...);
}


template <typename Check, typename... Ts>
constexpr auto AnyOf(Check check, TypeList<Ts...>) {
  return (check(std::type_identity<Ts>()) || ...);
}

template <typename T>
consteval auto IsSameCarried() {
  return []<typename T2>(std::type_identity<T2>){
    return std::is_same_v<T, T2>;
  };
}
template <typename T>
consteval auto IsConvertableCarried() {
  return []<typename T2>(std::type_identity<T2>){
    return std::is_convertible_v<T, T2>;
  };
}

struct Caster {
  inline constexpr Caster(auto) {}
  inline constexpr Caster() = default;
};

template <std::size_t... Is, typename T>
consteval auto Get(std::index_sequence<Is...>, decltype(Is, Caster())..., T&& arg, ...) -> T; 

template <std::size_t I, typename... Ts>
consteval auto Get(const TypeList<Ts...>&) -> decltype(Get(std::make_index_sequence<I>(), std::declval<Ts>()...));



} // namespace utils::impl

USERVER_NAMESPACE_END

