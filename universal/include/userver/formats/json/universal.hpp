
#pragma once
#include <userver/formats/universal/universal.hpp>
#include <userver/formats/json.hpp>
#include <type_traits>

USERVER_NAMESPACE_BEGIN
namespace formats::json {

template <typename T>
inline constexpr
std::enable_if_t<!std::is_same_v<decltype(universal::kSerialization<std::remove_cvref_t<T>>), const universal::impl::Disabled>, Value>
Serialize(T&& obj,
    serialize::To<Value>) {
  using Config = std::remove_const_t<decltype(universal::kSerialization<std::remove_cvref_t<T>>)>;
  using Type = std::remove_cvref_t<T>;
  return [&]<typename... Params>
      (universal::SerializationConfig<Type, Params...>){
    ValueBuilder builder;
    (universal::impl::UniversalSerializeField(Params{}, builder, obj), ...);
    return builder.ExtractValue();
  }(Config{});
};


} // namespace formats::json
USERVER_NAMESPACE_END
