#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {
struct ConfigDefault;
}  // namespace dynamic_config

namespace dynamic_config::impl {

std::string DoToJsonString(bool value);
std::string DoToJsonString(double value);
std::string DoToJsonString(std::uint64_t value);
std::string DoToJsonString(std::int64_t value);
std::string DoToJsonString(std::string_view value);

template <typename T, typename /*Unused*/>
struct IdWrapper {
  using type = T;
};

template <typename T, typename Unused>
using Id = typename IdWrapper<T, Unused>::type;

// Avoids including formats::json::Value and formats::json::ValueBuilder
// into frequently-used headers.
template <typename T, typename Value = formats::json::Value>
std::string ToJsonString(const T& value) {
  if constexpr (std::is_same_v<T, bool>) {
    return impl::DoToJsonString(static_cast<Id<bool, T>>(value));
  } else if constexpr (std::is_floating_point_v<T>) {
    return impl::DoToJsonString(static_cast<Id<double, T>>(value));
  } else if constexpr (std::is_unsigned_v<T>) {
    return impl::DoToJsonString(static_cast<Id<std::uint64_t, T>>(value));
  } else if constexpr (std::is_integral_v<T>) {
    return impl::DoToJsonString(static_cast<Id<std::int64_t, T>>(value));
  } else if constexpr (std::is_convertible_v<const T&, std::string_view>) {
    return impl::DoToJsonString(static_cast<Id<std::string_view, T>>(value));
  } else {
    return ToString(Serialize(value, formats::serialize::To<Value>{}));
  }
}

std::string SingleToDocsMapString(std::string_view name,
                                  std::string_view value);

std::string MultipleToDocsMapString(const ConfigDefault* data,
                                    std::size_t size);

template <typename T>
std::string ValueToDocsMapString(std::string_view name, const T& value) {
  return impl::SingleToDocsMapString(name, impl::ToJsonString(value));
}

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
