#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <userver/formats/json/serialize_container.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

class DocsMap final {
 public:
  /* Returns config item or throws an exception if key is missing */
  formats::json::Value Get(const std::string& name) const;

  void Set(std::string name, formats::json::Value);
  void Parse(const std::string& json, bool empty_ok);
  size_t Size() const;

  void MergeFromOther(DocsMap&& other);

  const std::unordered_set<std::string>& GetRequestedNames() const;
  std::unordered_set<std::string> GetNames() const;

  std::string AsJsonString() const;

  bool AreContentsEqual(const DocsMap& other) const;

 private:
  std::unordered_map<std::string, formats::json::Value> docs_;
  mutable std::unordered_set<std::string> requested_names_;
};

template <typename T>
class Value final {
 public:
  Value(const std::string& name, const DocsMap& docs_map)
      : value_(docs_map.Get(name).As<T>()) {}

  operator const T&() const { return value_; }

  const T& Get() const { return value_; }
  const T* operator->() const { return &value_; }

 private:
  T value_;
};

extern const std::string kValueDictDefaultName;

template <typename ValueType>
class ValueDict final {
 public:
  using DictType = std::unordered_map<std::string, ValueType>;
  using const_iterator = typename DictType::const_iterator;
  using iterator = const_iterator;
  using value_type = typename DictType::value_type;
  using key_type = std::string;
  using mapped_type = ValueType;

  ValueDict() = default;

  ValueDict(std::initializer_list<value_type> contents) : dict_(contents) {}

  ValueDict(DictType dict) : dict_(std::move(dict)) {}

  ValueDict(std::string name, DictType dict)
      : name_(std::move(name)), dict_(std::move(dict)) {}

  // Deprecated
  ValueDict(std::string name, const DocsMap& docs_map)
      : name_(std::move(name)),
        dict_(docs_map.Get(name_).template As<DictType>()) {}

  bool HasDefaultValue() const { return HasValue(kValueDictDefaultName); }

  template <typename StringType>
  bool HasValue(const StringType& key) const {
    return dict_.find(ToStringKey(key)) != dict_.end();
  }

  const ValueType& GetDefaultValue() const {
    const auto it = dict_.find(kValueDictDefaultName);
    if (it == dict_.end()) {
      throw std::runtime_error("no value for '" + kValueDictDefaultName + '\'' +
                               (name_.empty() ? "" : " in " + name_));
    }
    return it->second;
  }

  template <typename StringType>
  const ValueType& operator[](const StringType& key) const {
    auto it = dict_.find(ToStringKey(key));
    if (it == dict_.end()) {
      it = dict_.find(kValueDictDefaultName);
      if (it == dict_.end())
        throw std::runtime_error("no value for '" + ToStringKey(key) + '\'' +
                                 (name_.empty() ? "" : " in " + name_));
    }
    return it->second;
  }

  template <typename StringType>
  const ValueType& operator[](const std::optional<StringType>& key) const {
    if (key) return (*this)[*key];
    return GetDefaultValue();
  }

  template <typename StringType>
  const ValueType& Get(const StringType& key) const {
    return (*this)[key];
  }

  template <typename StringType>
  std::optional<ValueType> GetOptional(const StringType& key) const {
    auto it = dict_.find(ToStringKey(key));
    if (it == dict_.end()) {
      it = dict_.find(kValueDictDefaultName);
      if (it == dict_.end()) return std::nullopt;
    }

    return it->second;
  }

  auto begin() const { return dict_.begin(); }

  auto end() const { return dict_.end(); }

  const std::string& GetName() const { return name_; }

  bool operator==(const ValueDict& r) const { return dict_ == r.dict_; }

  bool operator!=(const ValueDict& r) const { return !(*this == r); }

 private:
  static const std::string& ToStringKey(const std::string& key) { return key; }

  template <typename StringType>
  static std::string ToStringKey(const StringType& key) {
    return std::string{std::string_view{key}};
  }

  std::string name_;
  DictType dict_;
};

template <typename T>
ValueDict<T> Parse(const formats::json::Value& value,
                   formats::parse::To<ValueDict<T>>) {
  return ValueDict<T>{value.GetPath(),
                      value.As<typename ValueDict<T>::DictType>()};
}

}  // namespace dynamic_config

USERVER_NAMESPACE_END
