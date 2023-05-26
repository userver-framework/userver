#pragma once

#include <optional>
#include <string>
#include <unordered_set>

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/impl/transparent_hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

class DocsMap final {
 public:
  /* Returns config item or throws an exception if key is missing */
  formats::json::Value Get(std::string_view name) const;

  void Set(std::string name, formats::json::Value);
  void Parse(const std::string& json, bool empty_ok);
  size_t Size() const;

  void MergeFromOther(DocsMap&& other);

  const std::unordered_set<std::string>& GetRequestedNames() const;
  std::unordered_set<std::string> GetNames() const;

  std::string AsJsonString() const;

  bool AreContentsEqual(const DocsMap& other) const;

 private:
  utils::impl::TransparentMap<std::string, formats::json::Value> docs_;
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

namespace impl {

[[noreturn]] void ThrowNoValueException(std::string_view dict_name,
                                        std::string_view key);

}  // namespace impl

template <typename ValueType>
class ValueDict final {
 public:
  using DictType = utils::impl::TransparentMap<std::string, ValueType>;
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

  bool HasValue(std::string_view key) const {
    return utils::impl::FindTransparent(dict_, key) != dict_.end();
  }

  const ValueType& GetDefaultValue() const {
    const auto it = dict_.find(kValueDictDefaultName);
    if (it == dict_.end()) {
      impl::ThrowNoValueException(name_, kValueDictDefaultName);
    }
    return it->second;
  }

  const ValueType& operator[](std::string_view key) const {
    auto it = utils::impl::FindTransparent(dict_, key);
    if (it == dict_.end()) {
      it = dict_.find(kValueDictDefaultName);
      if (it == dict_.end()) {
        impl::ThrowNoValueException(name_, key);
      }
    }
    return it->second;
  }

  template <typename StringType>
  const ValueType& operator[](const std::optional<StringType>& key) const {
    if (key) return (*this)[*key];
    return GetDefaultValue();
  }

  const ValueType& Get(std::string_view key) const { return (*this)[key]; }

  template <typename StringType>
  const ValueType& Get(const std::optional<StringType>& key) const {
    return (*this)[key];
  }

  std::optional<ValueType> GetOptional(std::string_view key) const {
    auto it = utils::impl::FindTransparent(dict_, key);
    if (it == dict_.end()) {
      it = dict_.find(kValueDictDefaultName);
      if (it == dict_.end()) return std::nullopt;
    }

    return it->second;
  }

  /// Sets the default value.
  /// The function is primarily there for testing purposes - ValueDict is
  /// normally obtained by parsing the config.
  void SetDefault(ValueType value) {
    Set(kValueDictDefaultName, std::move(value));
  }

  /// Sets a mapping. key == dynamic_config::kValueDictDefaultName is allowed.
  /// The function is primarily there for testing purposes - ValueDict is
  /// normally obtained by parsing the config.
  template <typename StringType>
  void Set(StringType&& key, ValueType value) {
    utils::impl::TransparentInsertOrAssign(dict_, std::forward<StringType>(key),
                                           std::move(value));
  }

  auto begin() const { return dict_.begin(); }

  auto end() const { return dict_.end(); }

  const std::string& GetName() const { return name_; }

  bool operator==(const ValueDict& r) const { return dict_ == r.dict_; }

  bool operator!=(const ValueDict& r) const { return !(*this == r); }

 private:
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
