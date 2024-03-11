#if __cplusplus >= 202002L
#pragma once
#include <userver/formats/universal/universal.hpp>
#include <userver/formats/common/items.hpp>
#include <userver/formats/parse/try_parse.hpp>
#include <format>
#include <unordered_map>
#include <userver/utils/regex.hpp>

USERVER_NAMESPACE_BEGIN
namespace formats::universal {
template <typename T>
struct Max : public impl::Param<T> {
  inline constexpr Max(const T& value) :
      impl::Param<T>(value) {}
  inline constexpr std::optional<std::string> Check(const T& value) const {
    if(value <= this->value) {
      return std::nullopt;
    }
    return this->Error(value);
  }
  template <typename Value>
  inline constexpr auto Check(const Value& value) const -> std::optional<std::string>
        requires meta::kIsRange<Value> {
    for(const auto& element : value) {
      if(element > this->value) {
        return this->Error(element);
      }
    }
    return std::nullopt;
  }
  inline constexpr std::string Error(const T& value) const {
    return std::format("{} > {}", value, this->value);
  }
};


struct MinElements : public impl::Param<std::size_t> {
  inline constexpr MinElements(const std::size_t& value) :
      impl::Param<std::size_t>(value) {}
  template <typename Value>
  inline constexpr auto Check(const Value& value) const -> std::optional<std::string> {
    if(value.size() >= this->value) {
      return std::nullopt;
    }
    return this->Error(value);
  }
  template <typename Value>
  inline constexpr auto Error(const Value&) const {
    return "Error";
  }
};

template <typename T>
struct Min : public impl::Param<T> {
  inline constexpr Min(const T& value) :
      impl::Param<T>(value) {}
  inline constexpr std::optional<std::string> Check(const T& value) const {
    if(value >= this->value) {
      return std::nullopt;
    }
    return this->Error(value);
  }
  template <typename Value>
  inline constexpr auto Check(const Value& value) const -> std::optional<std::string>
        requires meta::kIsRange<Value> {
    for(const auto& element : value) {
      if(element < this->value) {
        return this->Error(element);
      }
    }
    return std::nullopt;
  }
  inline constexpr auto Error(const T& value) const {
    return std::format("{} < {}", value, this->value);
  }
};
template <typename T, typename = void>
struct Default : public impl::EmptyCheck, private impl::Param<T(*)()> {
  inline constexpr Default(T (*ptr)()) :
      impl::Param<T(*)()>(ptr) {}
  inline constexpr Default(auto f) requires requires {static_cast<T(*)()>(f);} :
      impl::Param<T(*)()>(static_cast<T(*)()>(f)) {}
  inline constexpr auto value() const {
    return static_cast<const impl::Param<T(*)()>&>(*this).value();
  }
};

template <typename T>
struct Default<T, std::enable_if_t<std::is_arithmetic_v<T>>> : public impl::EmptyCheck, private impl::Param<T> {
  inline constexpr Default(const T& value) :
      impl::Param<T>(value) {}
  inline constexpr auto value() const {
    return static_cast<const impl::Param<T>&>(*this).value;
  }
};
template <>
struct Default<std::string, void> : public impl::EmptyCheck, private impl::Param<std::string_view> {
  inline constexpr Default(std::string_view value) :
      impl::Param<std::string_view>(value) {}
  template <std::size_t N>
  inline constexpr Default(const char(&value)[N]) :
      impl::Param<std::string_view>(value) {}
  inline constexpr auto value() const -> std::string {
    return static_cast<std::string>(static_cast<const impl::Param<std::string_view>&>(*this).value);
  }
};


template <utils::ConstexprString Pattern>
static const utils::regex kRegex(Pattern);

struct Pattern : public impl::EmptyCheck, public impl::Param<const utils::regex*> {
  constexpr inline Pattern(const utils::regex& regex) :
      impl::Param<const utils::regex*>(&regex) {}
  constexpr inline std::optional<std::string> Check(std::string_view str) const {
    if(utils::regex_match(str, *this->value)) {
      return std::nullopt;
    }
    return this->Error(str);
  }
  constexpr inline std::string Error(std::string_view) const {
    return "Error";
  }
};

struct AdditionalProperties : public impl::EmptyCheck, public impl::Param<bool> {
  constexpr inline AdditionalProperties(const bool& value) :
      impl::Param<bool>(value) {}
};

template <typename T>
struct FieldConfig<std::optional<T>> {
  FieldConfig<T> Main = {};
  std::optional<Default<T>> Default = std::nullopt;
  bool Required = false;
  bool Nullable = false;
  static inline constexpr auto Checker(const auto& check, const auto& value) -> std::optional<std::string> 
      requires requires{check.Check(*value);} {
    if(value) {
      return check.Check(*value);
    };
    return std::nullopt;
  }; 
  constexpr inline std::optional<std::string> Check(const std::optional<T>& value) const {
    if(value) {
      return this->Main.Check(*value);
    };
    return std::nullopt;
  }
  constexpr auto Write(const std::optional<T>& value, std::string_view field_name, const auto& names, auto& builder) const {
    if(value) {
      this->Main.Write(*value, field_name, names, builder);
      return;
    }
    if(this->Default) {
      this->Main.Write(this->Default->value(), field_name, names, builder);
      return;
    }
  }
  template <typename MainClass, auto I, typename Value>
  constexpr std::optional<T> Read(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    if(this->Required && value[name].IsMissing()) {
      return std::nullopt;
    }
    if(!value[name].IsMissing()) {
      return this->Nullable 
      ? (value[name].IsNull()
        ? std::nullopt
        : std::optional{this->Main.template Read<MainClass, I>(std::forward<Value>(value))})
      : std::optional{this->Main.template Read<MainClass, I>(std::forward<Value>(value))};
    }
    if(this->Default) {
      return this->Default->value();
    }
    return std::nullopt;
  }
  template <typename MainClass, auto I, typename Value>
  constexpr std::optional<std::optional<T>> TryRead(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    if(value[name].IsMissing() && this->Required) {
      return std::nullopt;
    }
    if(this->Nullable) {
      std::optional response = this->Main.template TryRead<MainClass, I>(std::forward<Value>(value));
      if(response) {
        return response;
      }
    } else {
      if(value[name].IsNull()) {
        return std::nullopt;
      };
      std::optional response = this->Main.template TryRead<MainClass, I>(std::forward<Value>(value));
      if(response) {
        return response;
      }
    }
    if(this->Default) {
      return this->Default->value();
    }
    return {{}};
  }
};

template <typename T>
struct FieldConfig<T, std::enable_if_t<meta::kIsInteger<T> || std::is_floating_point_v<T>>> {
  std::optional<Max<T>> Maximum = std::nullopt;
  std::optional<Min<T>> Minimum = std::nullopt;
  template <typename MainClass, auto I, typename Value>
  constexpr int Read(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    return value[name].template As<T>();
  }
  template <typename MainClass, auto I, typename Value>
  constexpr std::optional<T> TryRead(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    return parse::TryParse(value[name], parse::To<T>{});
  }
  constexpr auto Write(const int& value, std::string_view field_name, const auto&, auto& builder) const {
    builder[static_cast<std::string>(field_name)] = value;
  }
  inline constexpr std::optional<std::string> Check(const int&) const {
    return std::nullopt;
  }
};
template <>
struct FieldConfig<std::string> {
  std::optional<Pattern> Pattern = std::nullopt;
  template <typename MainClass, auto I, typename Value>
  constexpr std::string Read(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    return value[name].template As<std::string>();
  }
  template <typename MainClass, auto I, typename Value>
  constexpr std::optional<std::string> TryRead(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    return parse::TryParse(value[name], parse::To<std::string>{});
  }
  constexpr auto Write(std::string_view value, std::string_view field_name, const auto&, auto& builder) const {
    builder[static_cast<std::string>(field_name)] = value;
  }
  inline constexpr std::optional<std::string> Check(std::string_view) const {
    return std::nullopt;
  }

};

template <typename Value>
struct FieldConfig<std::unordered_map<std::string, Value>> {
  std::optional<AdditionalProperties> AdditionalProperties = std::nullopt;
  FieldConfig<Value> Items = {};
  using Type = std::unordered_map<std::string, Value>;
  template <typename MainClass, auto I, typename Value2>
  inline constexpr Type Read(Value2&& value) const {
    Type response;
    if(this->AdditionalProperties) {
      constexpr auto fields = boost::pfr::names_as_array<MainClass>();
      for(const auto& [name, value] : userver::formats::common::Items(std::forward<Value2>(value))) {
        auto it = std::find(fields.begin(), fields.end(), name);
        if(it == fields.end()) {
          response.emplace(name, value.template As<Value>());
        }
      }
      return response;
    }
    constexpr auto fieldName = boost::pfr::get_name<I, MainClass>();
    for(const auto& [name, value] : userver::formats::common::Items(value[fieldName])) {
      response.emplace(name, value.template As<Value>());
    }
    return response;
  }
  template <typename MainClass, auto I, typename Value2>
  inline constexpr std::optional<Type> TryRead(Value2&& value) const {
    Type response;
    if(this->AdditionalProperties) {
      constexpr auto fields = boost::pfr::names_as_array<MainClass>();
      for(const auto& [name, value] : userver::formats::common::Items(std::forward<Value2>(value))) {
        if(std::find(fields.begin(), fields.end(), name) == fields.end()) {
          auto New = parse::TryParse(value, parse::To<Value>{});
          if(!New) {
            return std::nullopt;
          }
          response.emplace(name, *New);
        }
      }
      return response;
    }
    constexpr auto field_name = boost::pfr::get_name<I, MainClass>();
    for(const auto& [name, value] : userver::formats::common::Items(value[field_name])) {
      auto New = parse::TryParse(value, parse::To<Value>{});
      if(!New) {
        return std::nullopt;
      }
      response.emplace(name, *New);
    }
    return response;
  }
  inline constexpr std::optional<std::string> Check(const Type& map) const {
    std::optional<std::string> error;
    for(const auto& [key, value] : map) {
      auto add = impl::UniversalCheckField(value, this->Items);
      if(add) {
        if(!error) {
          error = add;
          continue;
        }
        *error += *add;
      }
    }
    return error;
  }
  template <typename Builder>
  constexpr auto Write(const Type& value, std::string_view field_name, const auto&, Builder& builder) const {
    if(this->AdditionalProperties) {
      for(const auto& [name, value2] : value) {
        builder[name] = value2;
      }
      return;
    }
    Builder newBuilder;
    for(const auto& [name, value2] : value) {
      newBuilder[name] = value2;
    }
    builder[static_cast<std::string>(field_name)] = newBuilder.ExtractValue();
  }
};
template <typename Element>
struct FieldConfig<std::vector<Element>> {
  std::optional<MinElements> MinimalElements = std::nullopt;
  FieldConfig<Element> Items = {};
  template <typename MainClass, auto I, typename Value>
  inline constexpr auto Read(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    std::vector<Element> response;
    for(const auto& element : value[name]) {
      auto New = element.template As<Element>();
      response.push_back(std::move(New));
    }
    return response;
  }
  template <typename MainClass, auto I, typename Value>
  inline constexpr std::optional<std::vector<Element>> TryRead(Value&& value) const {
    std::vector<Element> response;
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    for(const auto& element : value[name]) {
      auto New = parse::TryParse(element, parse::To<Element>{});
      if(!New) {
        return std::nullopt;
      }
      response.push_back(std::move(*New));
    }
    return response;
  }
  inline constexpr std::optional<std::string> Check(const std::vector<Element>& obj) const {
    std::optional<std::string> error = std::nullopt;
    for(const auto& element : obj) {
      auto add = impl::UniversalCheckField(element, this->Items);
      if(add) {
        if(!error) {
          error = add;
          continue;
        }
        *error += *add;
      }
    }
    return error;
  }

};
} // namespace formats::universal
USERVER_NAMESPACE_END
#endif
