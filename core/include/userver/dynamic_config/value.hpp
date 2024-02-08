#pragma once

#include <optional>
#include <string>
#include <unordered_set>

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/impl/transparent_hash.hpp>
#include <userver/utils/internal_tag_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

class DocsMap final {
 public:
  /* Returns config item or throws an exception if key is missing */
  formats::json::Value Get(std::string_view name) const;

  bool Has(std::string_view name) const;
  void Set(std::string name, formats::json::Value);
  void Parse(std::string_view json_string, bool empty_ok);
  void Parse(formats::json::Value json, bool empty_ok);
  void Remove(const std::string& name);
  size_t Size() const;

  void MergeOrAssign(DocsMap&& source);
  void MergeMissing(const DocsMap& source);

  std::unordered_set<std::string> GetNames() const;
  formats::json::Value AsJson() const;
  bool AreContentsEqual(const DocsMap& other) const;

  /// @cond
  // For internal use only
  // Set of configs expected to be used is automatically updated when
  // configs are retrieved with 'Get' method.
  void SetConfigsExpectedToBeUsed(
      utils::impl::TransparentSet<std::string> configs, utils::InternalTag);

  const utils::impl::TransparentSet<std::string>& GetConfigsExpectedToBeUsed(
      utils::InternalTag) const;
  /// @endcond

 private:
  utils::impl::TransparentMap<std::string, formats::json::Value> docs_;
  mutable utils::impl::TransparentSet<std::string> configs_to_be_used_;
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

/// Name of the key with default value for dynamic_config::ValueDict
inline constexpr std::string_view kValueDictDefaultName = "__default__";

namespace impl {

[[noreturn]] void ThrowNoValueException(std::string_view dict_name,
                                        std::string_view key);

}  // namespace impl

/// @brief Dictionary that for missing keys falls back to a default value
/// stored by key dynamic_config::kValueDictDefaultName.
///
/// Usually used by dynamic configs for providing a fallback or default value
/// along with the specific values. For example:
/// @code{.json}
/// {
///  ".htm": "text/html",
///  ".html": "text/html",
///  "__default__": "text/plain"
/// }
/// @endcode
template <typename ValueType>
class ValueDict final {
 public:
  using DictType = utils::impl::TransparentMap<std::string, ValueType>;
  using const_iterator = typename DictType::const_iterator;
  using iterator = const_iterator;
  using value_type = typename DictType::value_type;
  using key_type = std::string;
  using mapped_type = ValueType;
  using init_list =
      std::initializer_list<std::pair<std::string_view, ValueType>>;

  ValueDict() = default;

  ValueDict(init_list contents) : dict_(contents.begin(), contents.end()) {}

  ValueDict(DictType dict) : dict_(std::move(dict)) {}

  ValueDict(std::string name, init_list contents)
      : name_(std::move(name)), dict_(contents.begin(), contents.end()) {}

  ValueDict(std::string name, DictType dict)
      : name_(std::move(name)), dict_(std::move(dict)) {}

  // Deprecated
  ValueDict(std::string_view name, const DocsMap& docs_map)
      : name_(name), dict_(docs_map.Get(name_).template As<DictType>()) {}

  /// Returns true if *this has a dynamic_config::kValueDictDefaultName key,
  /// otherwise returns false.
  bool HasDefaultValue() const noexcept {
    return HasValue(kValueDictDefaultName);
  }

  /// Returns true if *this has `key`, otherwise returns false.
  bool HasValue(std::string_view key) const noexcept {
    return utils::impl::FindTransparent(dict_, key) != dict_.end();
  }

  /// Returns value by dynamic_config::kValueDictDefaultName key,
  /// throws a std::runtime_error exception.
  const ValueType& GetDefaultValue() const {
    const auto it = utils::impl::FindTransparent(dict_, kValueDictDefaultName);
    if (it == dict_.end()) {
      impl::ThrowNoValueException(name_, kValueDictDefaultName);
    }
    return it->second;
  }

  /// Returns value by `key` if it is in *this, otherwise returns value
  /// by dynamic_config::kValueDictDefaultName if it is in *this,
  /// otherwise throws a std::runtime_error exception.
  const ValueType& operator[](std::string_view key) const {
    auto it = utils::impl::FindTransparent(dict_, key);
    if (it == dict_.end()) {
      it = utils::impl::FindTransparent(dict_, kValueDictDefaultName);
      if (it == dict_.end()) {
        impl::ThrowNoValueException(name_, key);
      }
    }
    return it->second;
  }

  /// Returns `key ? Get(*key) : GetDefaultValue()`
  template <typename StringType>
  const ValueType& operator[](const std::optional<StringType>& key) const {
    return key ? Get(*key) : GetDefaultValue();
  }

  /// @overload operator[](std::string_view key)
  const ValueType& Get(std::string_view key) const { return (*this)[key]; }

  /// Returns `key ? Get(*key) : GetDefaultValue()`
  template <typename StringType>
  const ValueType& Get(const std::optional<StringType>& key) const {
    return (*this)[key];
  }

  /// Returns value by `key` if it is in *this, otherwise returns value
  /// by dynamic_config::kValueDictDefaultName if it is in *this,
  /// otherwise returns an empty optional.
  std::optional<ValueType> GetOptional(std::string_view key) const {
    auto it = utils::impl::FindTransparent(dict_, key);
    if (it == dict_.end()) {
      it = utils::impl::FindTransparent(dict_, kValueDictDefaultName);
      if (it == dict_.end()) return std::nullopt;
    }

    return it->second;
  }

  /// Sets the default value.
  ///
  /// The function is primarily there for testing purposes - ValueDict is
  /// normally obtained by parsing the config.
  void SetDefault(ValueType value) {
    Set(kValueDictDefaultName, std::move(value));
  }

  /// Sets a mapping. key == dynamic_config::kValueDictDefaultName is allowed.
  ///
  /// The function is primarily there for testing purposes - ValueDict is
  /// normally obtained by parsing the config.
  template <typename StringType>
  void Set(StringType&& key, ValueType value) {
    utils::impl::TransparentInsertOrAssign(dict_, std::forward<StringType>(key),
                                           std::move(value));
  }

  auto begin() const noexcept { return dict_.begin(); }

  auto end() const noexcept { return dict_.end(); }

  const std::string& GetName() const noexcept { return name_; }

  bool operator==(const ValueDict& r) const noexcept {
    return dict_ == r.dict_;
  }

  bool operator!=(const ValueDict& r) const noexcept { return !(*this == r); }

 private:
  std::string name_;
  DictType dict_;
};

template <typename Value, typename T>
std::enable_if_t<formats::common::kIsFormatValue<Value>, ValueDict<T>> Parse(
    const Value& value, formats::parse::To<ValueDict<T>>) {
  return ValueDict<T>{value.GetPath(),
                      value.template As<typename ValueDict<T>::DictType>()};
}

}  // namespace dynamic_config

USERVER_NAMESPACE_END
