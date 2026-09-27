#if __cplusplus >= 202002L
#pragma once
#include <userver/utils/constexpr_string.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/parse/try_parse.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/utils/overloaded.hpp>
#include <variant>
#include <type_traits>
#include <boost/pfr/core.hpp>
#include <boost/pfr/core_name.hpp>
#include <boost/lexical_cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::universal {


template <typename T, typename Enable = void>
struct FieldConfig {
  constexpr inline std::optional<std::string> Check(const T&) const {
    return std::nullopt;
  }
  constexpr auto Write(const T& value, std::string_view field_name, const auto&, auto& builder) const {
    builder[static_cast<std::string>(field_name)] = value;
  }
  template <typename MainClass, auto I, typename Value>
  constexpr T Read(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    return value[name].template As<T>();
  }
  template <typename MainClass, auto I, typename Value>
  constexpr std::optional<T> TryRead(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    return parse::TryParse(value[name], parse::To<T>{});
  }
};
namespace impl {

template <typename T, std::size_t I>
using kFieldTypeOnIndex = std::remove_reference_t<decltype(boost::pfr::get<I>(std::declval<T>()))>;

template <typename T>
consteval std::size_t getFieldIndexByName(std::string_view field_name) {
  constexpr auto names = boost::pfr::names_as_array<T>();
  return std::find(names.begin(), names.end(), field_name) - names.begin();
}

struct Disabled {};


template <typename T>
struct Param {
  T value;
  constexpr Param(const T& value) :
      value(value) {}
};

template <auto Value>
struct Wrapper {
  static constexpr auto kValue = Value;
};

template <typename... Args>
inline constexpr std::optional<std::string> Add(Args&&... args) {
  std::optional<std::string> result;
  ([&]<typename T>(T&& arg){
    if(arg) {
      if(!result) {
        result.emplace(*std::forward<T>(arg));
        return;
      }
      result->append(*std::forward<T>(arg));
    }
  }(std::forward<Args>(args)), ...);
  return result;
}

template <typename T, typename TT>
inline constexpr auto checkCaller(T&& response, TT&& check) 
        requires (requires{FieldConfig<std::remove_cvref_t<T>>::Checker(check, response);} 
                || requires{check.Check(std::forward<T>(response));}) {
  using Type = FieldConfig<std::remove_cvref_t<T>>;
  if constexpr(requires{Type::Checker(check, response);}) {
    return Type::Checker(std::forward<TT>(check), response);
  } else {
    return check.Check(std::forward<T>(response));
  }
}

template <typename T>
using RemoveOptional = decltype(utils::Overloaded(
    []<typename TT>(const std::optional<TT>&) -> TT {},
    [](auto&&) -> T {}
  )(std::declval<const T&>()));


template <typename T, typename FieldConfigType>
inline constexpr std::optional<std::string> UniversalCheckField(
          T&& response,
          FieldConfig<FieldConfigType> config) {
  auto error = [&]<auto... Is>(std::index_sequence<Is...>) -> std::optional<std::string> {
    return Add([&]<auto II>(Wrapper<II>) -> std::optional<std::string> {
      const auto& check = boost::pfr::get<II>(config);
      using CheckType = std::remove_cvref_t<decltype(check)>;
      if constexpr(requires(RemoveOptional<CheckType> ch){checkCaller(response, ch);}) {
        if constexpr(meta::kIsOptional<CheckType>) {
          if(check) {
            std::optional error = checkCaller(response, *check);
            if(error) {
              return error;
            }
          }
        } else {
          std::optional error = checkCaller(response, check);
          if(error) {
            return error;
          }
        }
      }
      return std::nullopt;
    }(Wrapper<Is>{})...);
  }(std::make_index_sequence<boost::pfr::tuple_size_v<decltype(config)>>());
  if constexpr(meta::kIsOptional<std::remove_cvref_t<T>>) {
    if(response) {
      auto add = config.Check(*response);
      if(error) {
        if(add) {
          *error += *add;
        }
        return error;
      }
      error = add;
    }
  } else {
    auto add = config.Check(std::forward<T>(response));
    if(error) {
      if(add) {
        *error += *add;
      }
      return error;
    }
    error = add;
  }
  return error;
}


template <typename MainClass, auto I, typename T>
inline constexpr auto UniversalParseField(
          T&& value,
          FieldConfig<kFieldTypeOnIndex<MainClass, I>> config) {
  auto response = config.template Read<MainClass, I>(value);
  std::optional error = UniversalCheckField(response, config);
  if(error) {
    throw std::runtime_error(std::move(*error));
  }
  return response;
}

template <typename MainClass, auto I, typename T>
inline constexpr auto UniversalTryParseField(
          T&& value,
          FieldConfig<kFieldTypeOnIndex<MainClass, I>> config) {
  auto response = config.template TryRead<MainClass, I>(value);
  return [&]() -> decltype(response) {
    if(!UniversalCheckField(response, std::move(config))) {
      return response;
    }
    return std::nullopt;
  }();
}

template <typename MainClass, auto I, typename T, typename Builder>
inline constexpr auto UniversalSerializeField(
        T&& value,
        Builder& builder,
        FieldConfig<kFieldTypeOnIndex<MainClass, I>> config) {
  std::optional error = UniversalCheckField(value, config);
  if(error) {
    throw std::runtime_error(std::move(*error));
  }
  config.Write(std::forward<T>(value), boost::pfr::get_name<I, MainClass>(), boost::pfr::names_as_array<MainClass>(), builder);
}
struct EmptyCheck {
  template <typename Value>
  inline constexpr std::optional<std::string> Check(Value&&) const {
    return std::nullopt;
  }
};

template <typename T>
concept kEnabled = !(std::same_as<std::remove_cvref_t<T>, Disabled>);



} // namespace impl

template <typename T>
inline constexpr impl::Disabled kSerialization;

template <typename T>
inline constexpr auto kDeserialization = kSerialization<T>;


template <typename T>
class SerializationConfig {
  using FieldsConfigType = decltype([]<auto... I>(std::index_sequence<I...>){
    return std::type_identity<std::tuple<FieldConfig<impl::kFieldTypeOnIndex<T, I>>...>>();
  }(std::make_index_sequence<boost::pfr::tuple_size_v<T>>()))::type;

  public:
    template <utils::ConstexprString fieldName>
    inline constexpr auto& With(FieldConfig<impl::kFieldTypeOnIndex<T, impl::getFieldIndexByName<T>(fieldName)>>&& field_config) {
      constexpr auto Index = impl::getFieldIndexByName<T>(fieldName);
      std::get<Index>(this->fields_config) = std::move(field_config);
      return *this;
    }
    inline constexpr SerializationConfig() = default;

    template <std::size_t I>
    inline constexpr auto Get() const {
      return std::get<I>(this->fields_config);
    }
  private:
    FieldsConfigType fields_config = {};
};

template <typename... Ts>
class SerializationConfig<std::variant<Ts...>> {
  public:
    template <std::size_t I>
    inline constexpr auto& With(FieldConfig<decltype(Get<I>(utils::impl::TypeList<Ts...>{}))>&& field_config) {
      std::get<I>(this->variant_config) = std::move(field_config);
      return *this;
    }
    template <typename T>
    inline constexpr auto& With(FieldConfig<T>&& field_config) {
      return this->With<Find<T>(utils::impl::TypeList<Ts...>{})>(std::move(field_config));
    }
    inline constexpr SerializationConfig() = default;
  private:
    std::tuple<FieldConfig<Ts>...> variant_config = {};
};


} // namespace formats::universal


namespace formats::enums {

template <typename T>
inline constexpr auto kEnumMapping = universal::impl::Disabled{};


} // namespace formats::enums

namespace formats::parse {

template <typename To>
inline constexpr auto ParseFromString(std::string_view from) -> To {
  return boost::lexical_cast<To>(from);
}

template <>
inline constexpr auto ParseFromString<std::string_view>(std::string_view from) -> std::string_view {
  return from;
};

template <typename T>
inline constexpr auto ParseFromString(std::string_view value) -> T
    requires universal::impl::kEnabled<decltype(enums::kEnumMapping<T>)> {
  using M = decltype(enums::kEnumMapping<std::remove_cvref_t<T>>);
  auto response = enums::kEnumMapping<T>.TryFind(ParseFromString<typename M::template MappedTypeFor<T>>(value));
  if(!response) {
    throw std::runtime_error{"Can't serialize enum to string"};
  }
  return *std::move(response);
}

template <typename Value, typename T>
inline constexpr auto Parse(Value&& value, To<T>) -> T
      requires universal::impl::kEnabled<decltype(universal::kDeserialization<T>)> {
  return [&]<auto... I>(std::index_sequence<I...>){
    auto config = universal::kDeserialization<T>;
    return T{universal::impl::UniversalParseField<T, I>(value, config.template Get<I>())...};
  }(std::make_index_sequence<boost::pfr::tuple_size_v<T>>());
}

template <typename Value, typename T>
inline constexpr auto Parse(Value&& value, To<T>) -> T
    requires universal::impl::kEnabled<decltype(enums::kEnumMapping<T>)> {
  using M = decltype(enums::kEnumMapping<T>);
  constexpr auto f = [](auto response){
    if(response) {
      return *response;
    };
    throw std::runtime_error("Can't Parse to enum");
  };
  if constexpr(std::same_as<typename M::template MappedTypeFor<T>, std::string_view>) {
    return f(enums::kEnumMapping<T>.TryFind(value.template As<std::string>()));
  } else {
    return f(enums::kEnumMapping<T>.TryFind(value.template As<typename M::template MappedTypeFor<T>>()));
  };
}

template <typename Value, typename T>
inline constexpr auto TryParse(Value&& value, To<T>) -> std::optional<T>
      requires universal::impl::kEnabled<decltype(universal::kDeserialization<T>)> {
  return [&]<auto... I>(std::index_sequence<I...>) -> std::optional<T> {
    auto config = universal::kDeserialization<T>;
    auto tup = std::make_tuple(universal::impl::UniversalTryParseField<T, I>(value, config.template Get<I>())...);
    if((std::get<I>(tup) && ...)) {
      return T{*std::get<I>(tup)...};
    }
    return std::nullopt;
  }(std::make_index_sequence<boost::pfr::tuple_size_v<T>>());
}

} // namespace formats::parse
namespace formats::serialize {

template <typename From>
inline constexpr auto ToString(From&& from) -> std::string {
  return std::to_string(std::forward<From>(from));
}

inline constexpr auto ToString(std::string_view from) -> std::string {
  return static_cast<std::string>(from);
};

template <typename T>
inline constexpr auto ToString(T&& value) -> std::string 
    requires universal::impl::kEnabled<decltype(enums::kEnumMapping<std::remove_cvref_t<T>>)> {
  auto response = enums::kEnumMapping<std::remove_cvref_t<T>>.TryFind(std::forward<T>(value));
  if(!response) {
    throw std::runtime_error{"Can't serialize enum to string"};
  }
  return ToString(*std::move(response));
}



template <typename Value, typename T>
inline constexpr auto Serialize(T&& obj, To<Value>) -> Value 
    requires universal::impl::kEnabled<decltype(enums::kEnumMapping<std::remove_cvref_t<T>>)> {
  auto finded = enums::kEnumMapping<std::remove_cvref_t<T>>.TryFind(std::forward<T>(obj));
  if(!finded) {
    throw std::runtime_error("Can't serialize enum");
  }
  return typename Value::Builder(*std::move(finded)).ExtractValue();
}

template <typename Value, typename T>
inline constexpr auto Serialize(T&& obj, To<Value>) -> Value 
      requires universal::impl::kEnabled<decltype(universal::kSerialization<std::remove_cvref_t<T>>)> {
  using Type = std::remove_cvref_t<T>;
  return [&]<auto... I>(std::index_sequence<I...>){
    typename Value::Builder builder;
    auto config = universal::kSerialization<Type>;
    (universal::impl::UniversalSerializeField<Type, I>(boost::pfr::get<I>(obj), builder, config.template Get<I>()), ...);
    return builder.ExtractValue();
  }(std::make_index_sequence<boost::pfr::tuple_size_v<Type>>());
}

} // namespace formats::serialize

USERVER_NAMESPACE_END
#endif
