#if __cplusplus >= 202002L
#pragma once
#include <userver/utils/constexpr_string.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/parse/try_parse.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/utils/meta.hpp>
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
  };
  template <typename MainClass, auto I, typename Value>
  constexpr T Read(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    return value[name].template As<T>();
  };
  template <typename MainClass, auto I, typename Value>
  constexpr std::optional<T> TryRead(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    return parse::TryParse(value[name], parse::To<T>{});
  };
};
namespace impl {

template <typename T, std::size_t I>
using kFieldTypeOnIndex = std::remove_reference_t<decltype(boost::pfr::get<I>(std::declval<T>()))>;

template <typename T>
consteval std::size_t getFieldIndexByName(std::string_view fieldName) {
  constexpr auto names = boost::pfr::names_as_array<T>();
  return std::find(names.begin(), names.end(), fieldName) - names.begin();
};

struct Disabled {};


template <typename T>
struct Param {
  T value;
  constexpr Param(const T& value) :
      value(value) {};
};

template <auto Value>
struct Wrapper {
  static constexpr auto kValue = Value;
};



template <typename T>
inline constexpr auto RemoveOptional(const std::optional<T>& value) {
  auto response = *value;
  if constexpr(meta::kIsOptional<decltype(response)>) {
    return RemoveOptional(response);
  } else {
    return response;
  }
}
template <typename T>
inline constexpr auto RemoveOptional(const T& value) {
  return value;
}


template <typename T, typename T2, typename = void>
struct HasCheck : public std::false_type {};

template <typename T, typename T2>
struct HasCheck<T, T2, std::void_t<decltype(std::declval<T&>().Check(std::declval<T2&>()))>> : public std::true_type {};

template <typename... Args>
inline constexpr std::optional<std::string> Add(Args&&...) {
  return std::nullopt;
};

template <typename Arg, typename... Args>
inline constexpr std::optional<std::string> Add(Arg&& arg, Args&&... args) {
  if(arg) {
    auto toAdd = Add(std::forward<Args>(args)...);
    if(toAdd) {
      return *arg + *toAdd;
    };
    return *arg;
  };
  return Add(std::forward<Arg>(args)...);
};


template <typename T, typename FieldConfigType>
inline constexpr std::optional<std::string> UniversalCheckField(
          T&& response,
          FieldConfig<FieldConfigType> config) {
  using FieldType = std::remove_cvref_t<decltype(response)>;
  auto error = [&]<auto... Is>(std::index_sequence<Is...>) -> std::optional<std::string> {
    auto f = [&]<auto II>(Wrapper<II>) -> std::optional<std::string> {
      auto check = boost::pfr::get<II>(config);
      if constexpr(HasCheck<decltype(RemoveOptional(check)), decltype(RemoveOptional(response))>::value) {
        if constexpr(meta::kIsOptional<FieldType>) {
          if(response && check) {
            auto error = check->Check(RemoveOptional(response));
            if(error) {
              return error;
            };
          }
        } else {
          if(check) {
            auto error = check->Check(response);
            if(error) {
              return *error;
            }
          }
        }
      }
      return std::nullopt;
    };
    return Add(f(Wrapper<Is>{})...);
  }(std::make_index_sequence<boost::pfr::tuple_size_v<decltype(config)>>());
  if constexpr(meta::kIsOptional<std::remove_cvref_t<T>>) {
    if(response) {
      auto add = config.Check(*response);
      if(error) {
        if(add) {
          *error += *add;
        };
        return error;
      };
      error = add;
    }
  } else {
    auto add = config.Check(std::forward<T>(response));
    if(error) {
      if(add) {
        *error += *add;
      };
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
  const auto error = UniversalCheckField(response, config);
  if(error) {
    throw std::runtime_error(*error);
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
  UniversalCheckField(value, config);
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
    constexpr SerializationConfig() : fieldsConfig({}) {};

    template <std::size_t I>
    constexpr auto Get() const {
      return std::get<I>(this->fieldsConfig);
    };
  private:
    kFieldsConfigType fieldsConfig;

};



} // namespace formats::universal



namespace formats::parse {

template <typename Value, typename T>
inline constexpr std::enable_if_t<!std::is_same_v<std::remove_cvref_t<decltype(universal::kDeserialization<T>)>, universal::impl::Disabled>, T>
Parse(Value&& value, To<T>) {
  return [&]<auto... I>(std::index_sequence<I...>){
    auto config = universal::kSerialization<T>;
    return T{universal::impl::UniversalParseField<T, I>(value, config.template Get<I>())...};
  }(std::make_index_sequence<boost::pfr::tuple_size_v<T>>());
}

template <typename Value, typename T>
inline constexpr std::enable_if_t<!std::is_same_v<std::remove_cvref_t<decltype(universal::kDeserialization<T>)>, universal::impl::Disabled>, std::optional<T>>
TryParse(Value&& value, To<T>) {
  return [&]<auto... I>(std::index_sequence<I...>) -> std::optional<T> {
    auto config = universal::kSerialization<T>;
    auto tup = std::make_tuple(universal::impl::UniversalTryParseField<T, I>(value, config.template Get<I>())...);
    if((std::get<I>(tup) && ...)) {
      return T{*std::get<I>(tup)...};
    };
    return std::nullopt;
  }(std::make_index_sequence<boost::pfr::tuple_size_v<T>>());
}

} // namespace formats::parse
namespace formats::serialize {

template <typename Value, typename T>
inline constexpr std::enable_if_t<!std::is_same_v<decltype(universal::kSerialization<std::remove_cvref_t<T>>), universal::impl::Disabled>, Value>
Serialize(T&& obj, To<Value>) {
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
