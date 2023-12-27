#pragma once
#include <userver/formats/universal/universal.hpp>
#include <userver/formats/common/items.hpp>
#include <userver/formats/parse/try_parse.hpp>
#include <format>
#include <userver/utils/regex.hpp>

USERVER_NAMESPACE_BEGIN
namespace formats::universal {
template <typename T>
struct Max : public impl::Param<T> {
  inline constexpr Max(const T& value) :
      impl::Param<T>(value) {}
  inline constexpr std::string Check(const T& value) const {
    return value <= this->value ? "" : this->Error(value);
  }
  template <typename Value>
  inline constexpr std::enable_if_t<meta::kIsRange<Value>, std::string>
  Check(const Value& value) const {
    for(const auto& element : value) {
      if(element > this->value) {
        return this->Error(element);
      }
    }
    return "";
  }
  inline constexpr std::string Error(const T& value) const {
    return std::format("{} > {}", value, this->value);
  }
};

struct MinElements : public impl::Param<std::size_t> {
  inline constexpr MinElements(const std::size_t& value) :
      impl::Param<std::size_t>(value) {}
  template <typename Value>
  inline constexpr std::string Check(const Value& value) const {
    if(value.size() >= this->value) {
      return "";
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
  inline constexpr std::string Check(const T& value) const {
    return value >= this->value ? "" : this->Error(value);
  };
  template <typename Value>
  inline constexpr std::enable_if_t<meta::kIsRange<Value>, std::string>
  Check(const Value& value) const {
    for(const auto& element : value) {
      if(element < this->value) {
        return this->Error(element);
      }
    }
    return "";
  }
  inline constexpr auto Error(const T& value) const {
    return std::format("{} < {}", value, this->value);
  };
};
template <typename T>
struct Default : public impl::EmptyCheck, public impl::Param<T> {
  inline constexpr Default(const T& value) :
      impl::Param<T>(value) {}
};
template <utils::ConstexprString Pattern>
static const utils::regex kRegex(Pattern);

struct Pattern : public impl::EmptyCheck, public impl::Param<const utils::regex*> {
  constexpr inline Pattern(const utils::regex& regex) :
      impl::Param<const utils::regex*>(&regex) {}
  constexpr inline std::string Check(std::string_view str) const {
    return utils::regex_match(str, *this->value) ? "" : this->Error(str);
  }
  constexpr inline std::string Error(std::string_view) const {
    return "Error";
  }
};
struct Additional : public impl::EmptyCheck, public impl::Param<bool> {
  constexpr inline Additional(const bool& value) :
      impl::Param<bool>(value) {}
};
template <>
struct FieldConfig<int> {
  std::optional<Max<int>> Maximum;
  std::optional<Min<int>> Minimum;
  template <typename MainClass, auto I, typename Value>
  constexpr int Read(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    return value[name].template As<int>();
  };
  template <typename MainClass, auto I, typename Value>
  constexpr std::optional<int> TryRead(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    return parse::TryParse(value[name], parse::To<int>{});
  };
  constexpr auto Write(const int& value, std::string_view fieldName, const auto&, auto& builder) const {
    builder[static_cast<std::string>(fieldName)] = value;
  };
  inline constexpr std::string_view Check(const int&) const {
    return "";
  }

};
template <>
struct FieldConfig<std::optional<std::string>> {
  std::optional<Pattern> Pattern;
  std::optional<Default<std::string>> Default;
  template <typename MainClass, auto I, typename Value>
  constexpr std::optional<std::string> Read(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    if(!value.HasMember(name)) {
      return std::nullopt;
    };
    return value[name].template As<std::string>();
  };
  template <typename MainClass, auto I, typename Value>
  constexpr std::optional<std::optional<std::string>> TryRead(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    auto response = parse::TryParse(value[name], parse::To<std::string>{});
    if(response) {
      return response;
    }
    if(this->Default) {
      return this->Default->value;
    }
    return std::nullopt;
  }
  constexpr auto Write(const std::optional<std::string>& value, std::string_view fieldName, const auto&, auto& builder) const {
    if(value) {
      builder[static_cast<std::string>(fieldName)] = *value;
    };
  };
  inline constexpr std::string_view Check(const std::string&) const {
    return "";
  }

};
template <>
struct FieldConfig<std::string> {
  std::optional<Pattern> Pattern;
  template <typename MainClass, auto I, typename Value>
  constexpr std::string Read(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    return value[name].template As<std::string>();
  };
  template <typename MainClass, auto I, typename Value>
  constexpr std::optional<std::string> TryRead(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    return parse::TryParse(value[name], parse::To<std::string>{});
  };
  constexpr auto Write(std::string_view value, std::string_view fieldName, const auto&, auto& builder) const {
    builder[static_cast<std::string>(fieldName)] = value;
  };
  inline constexpr std::string_view Check(std::string_view) const {
    return "";
  }

};

template <>
struct FieldConfig<std::optional<int>> {
  std::optional<Max<int>> Maximum;
  std::optional<Min<int>> Minimum;
  std::optional<Default<int>> Default;
  template <typename MainClass, auto I, typename Value>
  constexpr std::optional<int> Read(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    if(value.HasMember(name)) {
      return value[name].template As<int>();
    }
    if(this->Default) {
      return this->Default->value;
    }
    return std::nullopt;
  }
  template <typename MainClass, auto I, typename Value>
  constexpr std::optional<std::optional<int>> TryRead(Value&& value) const {
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    auto response = parse::TryParse(value[name], parse::To<int>{});
    if(response) {
      return response;
    }
    if(this->Default) {
      return this->Default->value;
    }
    return {{}};
  }
  constexpr auto Write(const std::optional<int>& value, std::string_view fieldName, const auto&, auto& builder) const {
    if(value) {
      builder[static_cast<std::string>(fieldName)] = *value;
      return;
    }
    if(this->Default) {
      builder[static_cast<std::string>(fieldName)] = this->Default->value;
    }
  }

  inline constexpr std::string_view Check(const std::optional<int>&) const {
    return "";
  }

};
template <typename Value>
struct FieldConfig<std::unordered_map<std::string, Value>> {
  std::optional<Additional> Additional;
  using kType = std::unordered_map<std::string, Value>;
  template <typename MainClass, auto I, typename Value2>
  inline constexpr kType Read(Value2&& value) const {
    if(!this->Additional) {
      throw std::runtime_error("Invalid Flags");
    }
    kType response;
    constexpr auto fields = boost::pfr::names_as_array<MainClass>();
    for(const auto& [name, value2] : userver::formats::common::Items(std::forward<Value2>(value))) {
      auto it = std::find(fields.begin(), fields.end(), name);
      if(it == fields.end()) {
        response.emplace(name, value2.template As<Value>());
      }
    }
    return response;
  }
  template <typename MainClass, auto I, typename Value2>
  inline constexpr std::optional<kType> TryRead(Value2&& value) const {
    if(!this->Additional) {
      throw std::runtime_error("Invalid Flags");
    }
    kType response;
    constexpr auto fields = boost::pfr::names_as_array<MainClass>();
    constexpr auto name = boost::pfr::get_name<I, MainClass>();
    for(const auto& [name2, value2] : userver::formats::common::Items(std::forward<Value2>(value))) {
      if(std::find(fields.begin(), fields.end(), name2) == fields.end()) {
        auto New = parse::TryParse(value2, parse::To<Value>{});
        if(!New) {
          return std::nullopt;
        };
        response.emplace(name, *New);
      }
    }
    return response;
  }
  inline constexpr std::string_view Check(const kType&) const {
    return "";
  }
  constexpr auto Write(const kType& value, std::string_view, const auto&, auto& builder) const {
    for(const auto& [name, value2] : value) {
      builder[name] = value2;
    };
  };
};
template <typename Element>
struct FieldConfig<std::vector<Element>> {
  std::optional<MinElements> MinimalElements;
  FieldConfig<Element> Items;
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
  inline constexpr std::string Check(const std::vector<Element>& obj) const {
    std::string error;
    for(const auto& element : obj) {
      error += impl::UniversalCheckField(element, this->Items);
    }
    return error;
  }

};
} // namespace formats::universal
USERVER_NAMESPACE_END
