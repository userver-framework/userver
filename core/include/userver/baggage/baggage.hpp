#pragma once

/// @file userver/baggage/baggage.hpp
/// @brief @copybrief baggage::Baggage

#include <algorithm>  // TODO: remove
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <userver/engine/task/inherited_variable.hpp>

USERVER_NAMESPACE_BEGIN

namespace baggage {

using BaggageProperties =
    std::vector<std::pair<std::string, std::optional<std::string>>>;

class Baggage;
class BaggageEntryProperty;

/// @brief Baggage base exception
class BaggageException : public std::runtime_error {
 public:
  explicit BaggageException(const std::string& message)
      : std::runtime_error(message) {}
};

/// @brief property of entry. Has required key and optional value.
/// Keys shouldn't contain '=', ';' and ','. Values shouldn't contains
/// ',' and ';'
class BaggageEntryProperty {
  friend class BaggageEntry;

 public:
  BaggageEntryProperty(std::string_view key,
                       std::optional<std::string_view> value = std::nullopt);
  std::string ToString() const;
  std::optional<std::string> GetValue() const;
  std::string GetKey() const;

 private:
  const std::string_view key_;
  std::optional<std::string_view> value_;
};

/// @brief Baggage Entry. Has required key and value,
/// optional list of properties.
/// Keys shouldn't contain '=' and ','. Values shouldn't contains
/// ',' and ';'
class BaggageEntry {
  friend class Baggage;

 public:
  BaggageEntry(std::string_view key, std::string_view value,
               std::vector<BaggageEntryProperty> properties);
  std::string ToString() const;
  std::string GetValue() const;
  std::string GetKey() const;

  /// @return vector of properties of chosen entry
  const std::vector<BaggageEntryProperty>& GetProperties() const;

  /// @brief Check that entry contains property with selected key
  bool HasProperty(const std::string& key) const;

  /// @brief Get first property with selected key
  /// @throws BaggageException If key doesn't exist
  const BaggageEntryProperty& GetProperty(const std::string& key) const;

 private:
  /// @brief Add entry to the received header string
  void ConcatenateWith(std::string& header) const;
  const std::string_view key_;
  std::string_view value_;
  std::vector<BaggageEntryProperty> properties_;
};

/// @brief Baggage header. Contains entries (key, value, optional<properties>).
///
/// For more details on header check the official site
/// https://w3c.github.io/baggage/
///
/// @see baggage::BaggageManagerComponent
class Baggage {
 public:
  Baggage(std::string header, std::unordered_set<std::string> allowed_keys);
  Baggage(const Baggage&) noexcept;
  Baggage(Baggage&&) noexcept;

  std::string ToString() const;

  /// @return vector of entries
  const std::vector<BaggageEntry>& GetEntries() const;

  /// @brief check that header contains entry with selected key
  bool HasEntry(const std::string& key) const;

  /// @brief find first entry with selected key
  /// @throws BaggageException If key doesn't exist
  const BaggageEntry& GetEntry(const std::string& key) const;

  /// @brief entry's key validation
  bool IsValidEntry(const std::string& key) const;

  /// @brief Add entry to baggage header.
  /// @throws BaggageException If key, value or properties
  /// don't match with requirements or if allowed_keys
  /// don't contain selected key
  void AddEntry(std::string key, std::string value,
                BaggageProperties properties);

  /// @brief get baggage allowed keys
  std::unordered_set<std::string> GetAllowedKeys() const;

 protected:
  /// @brief parsers
  /// @returns std::nullopt If key, value or properties
  /// don't match with requirements or if allowed_keys
  /// don't contain selected key
  std::optional<BaggageEntry> TryMakeBaggageEntry(std::string_view entry);
  static std::optional<BaggageEntryProperty> TryMakeBaggageEntryProperty(
      std::string_view property);

 private:
  /// @brief Parse baggage_header and fill entries_
  void FillEntries();

  /// @brief Create result_header
  void CreateResultHeader();

  std::string header_value_;
  std::unordered_set<std::string> allowed_keys_;
  std::vector<BaggageEntry> entries_;

  // result header after parsing entities.
  // empty string if is_valid_header == true
  std::string result_header_;

  // true if requested header == header for sending
  bool is_valid_header_ = true;
};

/// @brief Parsing function
std::optional<Baggage> TryMakeBaggage(
    std::string header, std::unordered_set<std::string> allowed_keys);

template <typename T>
bool HasInvalidSymbols(const T& obj) {
  return std::find_if(obj.begin(), obj.end(), [](unsigned char x) {
           return x == ',' || x == ';' || x == '=' || std::isspace(x);
         }) != obj.end();
}

inline engine::TaskInheritedVariable<Baggage> kInheritedBaggage;

}  // namespace baggage

USERVER_NAMESPACE_END
