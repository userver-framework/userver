#pragma once

/// @file userver/utils/default_dict.hpp
/// @brief Dictionary with special value for missing keys

#include <optional>
#include <string>

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/impl/transparent_hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// Name of the key with default value for utils::DefaultDict
inline constexpr std::string_view kDefaultDictDefaultName = "__default__";

namespace impl {

[[noreturn]] void ThrowNoValueException(std::string_view dict_name,
                                        std::string_view key);

}  // namespace impl

/// @ingroup userver_universal userver_containers
///
/// @brief Dictionary that for missing keys falls back to a default value
/// stored by key utils::kDefaultDictDefaultName.
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
class DefaultDict final {
 public:
  using DictType = utils::impl::TransparentMap<std::string, ValueType>;
  using const_iterator = typename DictType::const_iterator;
  using iterator = const_iterator;
  using value_type = typename DictType::value_type;
  using key_type = std::string;
  using mapped_type = ValueType;
  using init_list =
      std::initializer_list<std::pair<std::string_view, ValueType>>;

  DefaultDict() = default;

  DefaultDict(init_list contents) : dict_(contents.begin(), contents.end()) {}

  DefaultDict(DictType dict) : dict_(std::move(dict)) {}

  DefaultDict(std::string name, init_list contents)
      : name_(std::move(name)), dict_(contents.begin(), contents.end()) {}

  DefaultDict(std::string name, DictType dict)
      : name_(std::move(name)), dict_(std::move(dict)) {}

  /// Returns true if *this has a utils::kDefaultDictDefaultName key,
  /// otherwise returns false.
  bool HasDefaultValue() const noexcept {
    return HasValue(kDefaultDictDefaultName);
  }

  /// Returns true if *this has `key`, otherwise returns false.
  bool HasValue(std::string_view key) const noexcept {
    return utils::impl::FindTransparent(dict_, key) != dict_.end();
  }

  /// Returns value by utils::kDefaultDictDefaultName key,
  /// throws a std::runtime_error exception.
  const ValueType& GetDefaultValue() const {
    const auto it =
        utils::impl::FindTransparent(dict_, kDefaultDictDefaultName);
    if (it == dict_.end()) {
      impl::ThrowNoValueException(name_, kDefaultDictDefaultName);
    }
    return it->second;
  }

  /// Returns value by `key` if it is in *this, otherwise returns value
  /// by utils::kDefaultDictDefaultName if it is in *this,
  /// otherwise throws a std::runtime_error exception.
  const ValueType& operator[](std::string_view key) const {
    auto it = utils::impl::FindTransparent(dict_, key);
    if (it == dict_.end()) {
      it = utils::impl::FindTransparent(dict_, kDefaultDictDefaultName);
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
  /// by utils::kDefaultDictDefaultName if it is in *this,
  /// otherwise returns an empty optional.
  std::optional<ValueType> GetOptional(std::string_view key) const {
    auto it = utils::impl::FindTransparent(dict_, key);
    if (it == dict_.end()) {
      it = utils::impl::FindTransparent(dict_, kDefaultDictDefaultName);
      if (it == dict_.end()) return std::nullopt;
    }

    return it->second;
  }

  /// Sets the default value.
  ///
  /// The function is primarily there for testing purposes - DefaultDict is
  /// normally obtained by parsing the config.
  void SetDefault(ValueType value) {
    Set(kDefaultDictDefaultName, std::move(value));
  }

  /// Sets a mapping. key == utils::kDefaultDictDefaultName is allowed.
  ///
  /// The function is primarily there for testing purposes - DefaultDict is
  /// normally obtained by parsing the config.
  template <typename StringType>
  void Set(StringType&& key, ValueType value) {
    utils::impl::TransparentInsertOrAssign(dict_, std::forward<StringType>(key),
                                           std::move(value));
  }

  auto begin() const noexcept { return dict_.begin(); }

  auto end() const noexcept { return dict_.end(); }

  const std::string& GetName() const noexcept { return name_; }

  bool operator==(const DefaultDict& r) const noexcept {
    return dict_ == r.dict_;
  }

  bool operator!=(const DefaultDict& r) const noexcept { return !(*this == r); }

 private:
  std::string name_;
  DictType dict_;
};

template <typename Value, typename T>
std::enable_if_t<formats::common::kIsFormatValue<Value>, DefaultDict<T>> Parse(
    const Value& value, formats::parse::To<DefaultDict<T>>) {
  return DefaultDict<T>{value.GetPath(),
                        value.template As<typename DefaultDict<T>::DictType>()};
}

}  // namespace utils

USERVER_NAMESPACE_END
