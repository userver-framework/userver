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

template <typename T, typename Value>
constexpr inline bool Is(Value&& value) {
  if constexpr(std::is_same_v<T, bool>) {
    return value.IsBool();
  } else if constexpr(meta::kIsInteger<T>) {
    return (std::is_unsigned_v<T> && sizeof(T) == sizeof(std::uint64_t)) ? value.IsUInt64() : value.IsInt64();
  } else if constexpr(std::is_convertible_v<T, std::string>) {
    return value.IsString();
  } else if constexpr(std::is_convertible_v<T, double>) {
    return value.IsDouble();
  } else if constexpr(meta::kIsRange<T>) {
    return value.IsArray();
  } else {
    return value.IsObject();
  }
}
bool CheckInBounds(const auto& x, const auto& min, const auto& max) {
  if (x < min || x > max) {
    return false;
  };
  return true;
};
inline constexpr utils::impl::TypeList<bool, double, std::string> kBaseTypes;

} // namespace impl


template <typename T, typename Value>
constexpr inline std::enable_if_t<
        utils::impl::AnyOf(utils::impl::IsSameCarried<T>(), impl::kBaseTypes) ||
        meta::kIsInteger<T>, std::optional<T>>
TryParse(Value&& value, userver::formats::parse::To<T>) {
  if(!impl::Is<T>(value)) {
    return std::nullopt;
  }
  return value.template As<T>();
}

template <typename T, typename Value>
constexpr inline std::optional<std::optional<T>> TryParse(Value&& value, userver::formats::parse::To<std::optional<T>>) {
  return TryParse(std::forward<Value>(value), userver::formats::parse::To<T>{});
}
template <typename Value>
constexpr inline std::optional<float> TryParse(Value&& value, userver::formats::parse::To<float>) {
  auto response = value.template As<double>();
  if(impl::CheckInBounds(response, std::numeric_limits<float>::lowest(),
                        std::numeric_limits<float>::max())) {
    return static_cast<float>(response);
  };
  return std::nullopt;
};

template <typename T, typename Value>
constexpr inline std::enable_if_t<meta::kIsRange<T> && !meta::kIsMap<T> &&
                     !std::is_same_v<T, boost::uuids::uuid> &&
                     !utils::impl::AnyOf(utils::impl::IsConvertableCarried<T>(), impl::kBaseTypes) &&
                     !std::is_convertible_v<
                         T&, utils::impl::strong_typedef::StrongTypedefTag&>,
                 std::optional<T>>
TryParse(Value&& from, To<T>) {
  T response;
  auto inserter = std::inserter(response, response.end());
  using ValueType = meta::RangeValueType<T>;
  for(const auto& item : from) {
    auto insert = TryParse(item, userver::formats::parse::To<ValueType>{});
    if(!insert) {
      return std::nullopt;
    };
    *inserter = *insert;
    ++inserter;
  };
  return response;
}



} // namespace formats::parse

USERVER_NAMESPACE_END
