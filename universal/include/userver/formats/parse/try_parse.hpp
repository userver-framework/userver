#pragma once
#include <type_traits>
#include <string>
#include <cstdint>
#include <userver/formats/parse/to.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/impl/type_list.hpp>
#include <userver/formats/common/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

namespace impl {

template <typename T, typename Value>
constexpr inline bool Is(Value&& value) {
  if constexpr(std::is_convertible_v<T, std::int64_t>) {
    return value.IsInt();
  } else if constexpr(std::is_convertible_v<T, std::string>) {
    return value.IsString();
  } else if constexpr(requires {std::begin(std::declval<T>());}) {
    return value.IsArray();
  } else if constexpr(std::is_convertible_v<bool, T>) {
    return value.IsBool();
  } else {
    return value.IsObject();
  }
}

inline constexpr utils::impl::TypeList<bool, std::int64_t, std::uint64_t, double, std::string> kBaseTypes;


} // namespace impl


template <typename T, typename Value>
constexpr inline std::enable_if_t<utils::impl::AnyOf(utils::impl::IsConvertableCarried<T>(), impl::kBaseTypes), std::optional<T>>
TryParse(Value&& value, userver::formats::parse::To<T>) {
  if(!impl::Is<T>(value)) {
    return std::nullopt;
  }
  auto obj = value.template As<std::optional<T>>();
  if(obj) {
    return obj;
  }
  return std::nullopt;
}

template <typename T, typename Value>
constexpr inline std::optional<std::optional<T>> TryParse(Value&& value, userver::formats::parse::To<std::optional<T>>) {
  auto object = TryParse(std::forward<Value>(value), userver::formats::parse::To<T>{});
  if(object) {
    return object;
  }
  return std::nullopt;
}

} // namespace formats::parse

USERVER_NAMESPACE_END
