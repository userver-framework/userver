#if __cplusplus >= 202002L
#pragma once
#include <userver/utils/constexpr_string.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/parse/try_parse.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/overloaded.hpp>
#include <type_traits>
#include <boost/pfr.hpp>


USERVER_NAMESPACE_BEGIN

namespace formats::universal {


template <typename T>
struct FieldConfig {
  constexpr inline std::optional<std::string> Check(const T&) const {
    return std::nullopt;
  }
  constexpr auto Write(const T& value, std::string_view fieldName, const auto&, auto& builder) const {
    builder[static_cast<std::string>(fieldName)] = value;
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
consteval std::size_t getFieldIndexByName(std::string_view fieldName) {
  constexpr auto names = boost::pfr::names_as_array<T>();
  return std::find(names.begin(), names.end(), fieldName) - names.begin();
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
inline constexpr std::optional<std::string> Add(Args&&...) {
  return std::nullopt;
}

template <typename Arg, typename... Args>
inline constexpr std::optional<std::string> Add(Arg&& arg, Args&&... args) {
  if(arg) {
    auto toAdd = Add(std::forward<Args>(args)...);
    if(toAdd) {
      return *arg + *toAdd;
    }
    return *arg;
  }
  return Add(std::forward<Arg>(args)...);
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
} // namespace impl

template <typename T>
inline constexpr impl::Disabled kSerialization;

template <typename T>
inline constexpr auto kDeserialization = kSerialization<T>;


template <typename T>
struct SerializationConfig {
  using kFieldsConfigType = decltype([]<auto... I>(std::index_sequence<I...>){
    return std::type_identity<std::tuple<FieldConfig<impl::kFieldTypeOnIndex<T, I>>...>>();
  }(std::make_index_sequence<boost::pfr::tuple_size_v<T>>()))::type;

  public:
    template <utils::ConstexprString fieldName>
    inline constexpr auto& With(FieldConfig<impl::kFieldTypeOnIndex<T, impl::getFieldIndexByName<T>(fieldName)>>&& fieldConfig) {
      constexpr auto Index = impl::getFieldIndexByName<T>(fieldName);
      static_assert(Index != boost::pfr::tuple_size_v<T>, "Field Not Found");
      std::get<Index>(this->fieldsConfig) = std::move(fieldConfig);
      return *this;
    }
    constexpr SerializationConfig() : fieldsConfig({}) {}

    template <std::size_t I>
    constexpr auto Get() const {
      return std::get<I>(this->fieldsConfig);
    }
  private:
    kFieldsConfigType fieldsConfig;

};



} // namespace formats::universal



namespace formats::parse {

template <typename Value, typename T>
inline constexpr auto Parse(Value&& value, To<T>) -> T
      requires (!std::same_as<std::remove_cvref_t<decltype(universal::kDeserialization<std::remove_cvref_t<T>>)>, universal::impl::Disabled>) {
  return [&]<auto... I>(std::index_sequence<I...>){
    auto config = universal::kDeserialization<T>;
    return T{universal::impl::UniversalParseField<T, I>(value, config.template Get<I>())...};
  }(std::make_index_sequence<boost::pfr::tuple_size_v<T>>());
}

template <typename Value, typename T>
inline constexpr auto TryParse(Value&& value, To<T>) -> std::optional<T>
      requires (!std::same_as<std::remove_cvref_t<decltype(universal::kDeserialization<std::remove_cvref_t<T>>)>, universal::impl::Disabled>) {
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

template <typename Value, typename T>
inline constexpr auto Serialize(T&& obj, To<Value>) -> Value 
      requires (!std::same_as<std::remove_cvref_t<decltype(universal::kSerialization<std::remove_cvref_t<T>>)>, universal::impl::Disabled>) {
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
