#pragma once
#include <type_traits>


USERVER_NAMESPACE_BEGIN

namespace utils::impl {

template <typename... T>
struct TypeList {};


template <typename... Ts>
consteval auto size(TypeList<Ts...>) {
  return sizeof...(Ts);
}


template <template <typename...> typename Check, typename... Ts>
consteval auto anyOf(TypeList<Ts...>) {
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

} // namespace utils::impl

USERVER_NAMESPACE_END

