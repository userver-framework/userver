#pragma once
#include <type_traits>
#include <string>
#include <cstdint>
#include <limits>
#include <userver/formats/parse/to.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/impl/type_list.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/formats/parse/common_containers.hpp>


USERVER_NAMESPACE_BEGIN

namespace formats::parse {

namespace impl {
bool CheckInBounds(const auto& x, const auto& min, const auto& max) {
  if (x < min || x > max) {
    return false;
  };
  return true;
};

} // namespace impl

template <typename Value, typename T>
inline constexpr std::optional<T> TryParse(Value&& value, To<T>) {
  if(!value.IsObject()) {
    return std::nullopt;
  }
  return value.template As<T>();
}

template <typename Value>
inline constexpr std::optional<bool> TryParse(Value&& value, To<bool>) {
  if(!value.IsBool()) {
    return std::nullopt;
  }
  return value.template As<bool>();
}
template <typename Value>
inline constexpr std::optional<double> TryParse(Value&& value, To<double>) {
  if(!value.IsDouble()) {
    return std::nullopt;
  }
  return value.template As<double>();
}
template <typename Value>
inline constexpr std::optional<std::string> TryParse(Value&& value, To<std::string>) {
  if(!value.IsString()) {
    return std::nullopt;
  }
  return value.template As<std::string>();
}

template <typename Value, typename T>
inline constexpr std::optional<T> TryParse(Value&& value, To<T>) requires meta::kIsInteger<T> {
  constexpr auto f = [](auto response){
    return impl::CheckInBounds(response, std::numeric_limits<T>::min(), std::numeric_limits<T>::max()) ? std::optional{static_cast<T>(response)} : std::nullopt;
  };
  if constexpr(std::is_unsigned_v<T>) {
    if(!value.IsUInt64()) {
      return std::nullopt;
    }
    auto response = value.template As<std::uint64_t>();
    return f(std::move(response));
  } else {
    if(!value.IsInt64()) {
      return std::nullopt;
    }
    auto response = value.template As<std::int64_t>();
    return f(std::move(response));
  }
}
template <typename Value, typename T>
inline constexpr std::optional<std::vector<T>> TryParse(Value&& value, To<std::vector<T>>) {
  if(!value.IsArray()) {
    return std::nullopt;
  }
  std::vector<T> response;
  response.reserve(value.GetSize());
  for(const auto& item : value) {
    auto insert = TryParse(item, To<T>{});
    if(!insert) {
      return std::nullopt;
    }
    response.push_back(std::move(*insert));
  }
  return response;
}

template <typename T, typename Value>
constexpr inline std::optional<std::optional<T>> TryParse(Value&& value, To<std::optional<T>>) {
  if (value.IsMissing() || value.IsNull()) {
    return std::optional<T>{std::nullopt};
  }
  return TryParse(value, To<T>{});
}

template <typename Value>
constexpr inline std::optional<float> TryParse(Value&& value, To<float>) {
  auto response = value.template As<double>();
  if(impl::CheckInBounds(response, std::numeric_limits<float>::lowest(),
                        std::numeric_limits<float>::max())) {
    return static_cast<float>(response);
  }
  return std::nullopt;
}



} // namespace formats::parse

USERVER_NAMESPACE_END
