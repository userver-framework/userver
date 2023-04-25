#include <userver/baggage/baggage.hpp>

#include <algorithm>

#include <fmt/format.h>

#include <userver/baggage/baggage_settings.hpp>
#include <userver/http/parser/http_request_parse_args.hpp>
#include <userver/http/url.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace baggage {

namespace {

const int kEntitiesLimit = 64;
const int kHeaderLengthLimit = 8192;

}  // namespace

BaggageEntryProperty::BaggageEntryProperty(
    std::string_view key, std::optional<std::string_view> value)
    : key_(std::move(key)), value_(std::move(value)) {}

std::string BaggageEntryProperty::ToString() const {
  if (value_) {
    return std::string(key_) + '=' + std::string(*value_);
  }
  return std::string(key_);
}

std::optional<std::string> BaggageEntryProperty::GetValue() const {
  if (!value_) {
    return std::nullopt;
  }
  return std::make_optional<std::string>(http::parser::UrlDecode(*value_));
}
std::string BaggageEntryProperty::GetKey() const {
  return http::parser::UrlDecode(key_);
}

BaggageEntry::BaggageEntry(std::string_view key, std::string_view value,
                           std::vector<BaggageEntryProperty> properties)
    : key_(std::move(key)),
      value_(std::move(value)),
      properties_(std::move(properties)) {}

std::string BaggageEntry::ToString() const {
  std::string entry = std::string(key_) + "=" + std::string(value_);
  for (const auto& property : properties_) {
    entry += ";" + property.ToString();
  }
  return entry;
}

void BaggageEntry::ConcatenateWith(std::string& header) const {
  header += std::string(key_) + "=" + std::string(value_);
  for (const auto& property : properties_) {
    header += ";" + property.ToString();
  }
}

std::string BaggageEntry::GetValue() const {
  return http::parser::UrlDecode(value_);
}
std::string BaggageEntry::GetKey() const {
  return http::parser::UrlDecode(key_);
}
const std::vector<BaggageEntryProperty>& BaggageEntry::GetProperties() const {
  return properties_;
}

bool BaggageEntry::HasProperty(const std::string& key) const {
  for (const auto& property : properties_) {
    if (property.key_ == http::UrlEncode(key)) {
      return true;
    }
  }
  return false;
}

const BaggageEntryProperty& BaggageEntry::GetProperty(
    const std::string& key) const {
  for (const auto& property : properties_) {
    if (property.key_ == http::UrlEncode(key)) {
      return property;
    }
  }
  throw BaggageException("Entry doesn't contain selected property");
}

std::string Baggage::ToString() const {
  if (is_valid_header_) {
    return header_value_;
  }
  return result_header_;
}

const std::vector<BaggageEntry>& Baggage::GetEntries() const {
  return entries_;
}

bool Baggage::HasEntry(const std::string& key) const {
  for (const auto& entry : entries_) {
    if (entry.key_ == http::UrlEncode(key)) {
      return true;
    }
  }
  return false;
}

const BaggageEntry& Baggage::GetEntry(const std::string& key) const {
  for (const auto& entry : entries_) {
    if (entry.key_ == http::UrlEncode(key)) {
      return entry;
    }
  }
  throw BaggageException("Baggage doesn't contain selected entry");
}

Baggage::Baggage(std::string header,
                 std::unordered_set<std::string> allowed_keys)
    : header_value_(std::move(header)), allowed_keys_(std::move(allowed_keys)) {
  header_value_.erase(
      std::remove_if(header_value_.begin(), header_value_.end(),
                     [](unsigned char x) { return std::isspace(x); }),
      header_value_.end());

  FillEntries();

  // if header contains invalid symbols, we should fill result_header_
  if (!is_valid_header_) {
    CreateResultHeader();
  }
}

Baggage::Baggage(const Baggage& baggage_copy) noexcept
    : allowed_keys_(baggage_copy.allowed_keys_) {
  if (baggage_copy.is_valid_header_) {
    header_value_ = baggage_copy.header_value_;
  } else {
    header_value_ = baggage_copy.result_header_;
  }
  FillEntries();

  // if header contains invalid symbols, we should fill result_header_
  if (!is_valid_header_) {
    CreateResultHeader();
  }
}

Baggage::Baggage(Baggage&& baggage_copy) noexcept
    : allowed_keys_(std::move(baggage_copy.allowed_keys_)) {
  if (baggage_copy.is_valid_header_) {
    header_value_ = std::move(baggage_copy.header_value_);
  } else {
    header_value_ = std::move(baggage_copy.result_header_);
  }
  FillEntries();

  // if header contains invalid symbols, we should fill result_header_
  if (!is_valid_header_) {
    CreateResultHeader();
  }
}

void Baggage::AddEntry(std::string key, std::string value,
                       BaggageProperties properties) {
  auto encoded_key = http::UrlEncode(key);
  auto encoded_value = http::UrlEncode(value);
  if (!IsValidEntry(key)) {
    throw BaggageException(
        "White list of entries doesn't contain selected key");
  }
  if (entries_.size() >= kEntitiesLimit) {
    throw BaggageException(
        fmt::format("Exceeded the limit of entries: {}", kEntitiesLimit));
  }

  std::string entry;
  if (!entries_.empty()) {
    entry += ',';
  }
  entry += encoded_key + '=' + encoded_value;

  for (auto&& property : properties) {
    auto encoded_property_key = http::UrlEncode(property.first);
    entry += ';' + encoded_property_key;
    if (property.second) {
      auto encoded_property_value = http::UrlEncode(*property.second);
      entry += '=' + encoded_property_value;
    }
  }
  if (is_valid_header_) {
    if (header_value_.size() + entry.size() >= kHeaderLengthLimit) {
      throw BaggageException(fmt::format(
          "Exceeded the limit of header length: {}", kHeaderLengthLimit));
    }
    header_value_ += entry;
    entries_.clear();
    FillEntries();

    // if header contains invalid symbols, we should fill result_header_
    if (!is_valid_header_) {
      CreateResultHeader();
    }
  } else {
    if (result_header_.size() + entry.size() >= kHeaderLengthLimit) {
      throw BaggageException(fmt::format(
          "Exceeded the limit of header length: {}", kHeaderLengthLimit));
    }
    // if header contains invalid entities, we should make new header
    // by concatenating result_header_ and new entry
    header_value_ = result_header_ + entry;
    is_valid_header_ = true;
    entries_.clear();
    FillEntries();

    // if header contains invalid symbols, we should fill result_header_
    if (!is_valid_header_) {
      CreateResultHeader();
    }
  }
}

bool Baggage::IsValidEntry(const std::string& key) const {
  return allowed_keys_.count(key);
}

std::unordered_set<std::string> Baggage::GetAllowedKeys() const {
  return allowed_keys_;
}

void Baggage::CreateResultHeader() {
  result_header_.reserve(header_value_.size());
  for (size_t i = 0; i < entries_.size(); i++) {
    if (i != 0) {
      result_header_ += ",";
    }
    entries_[i].ConcatenateWith(result_header_);
  }
}

void Baggage::FillEntries() {
  entries_.reserve(kEntitiesLimit);
  for (size_t header_pos = 0;
       header_pos != std::string::npos && entries_.size() < kEntitiesLimit;) {
    std::string_view entry{header_value_};
    entry.remove_prefix(header_pos);
    header_pos = header_value_.find(',', header_pos);
    if (header_pos != std::string::npos) {
      entry.remove_suffix(header_value_.size() - header_pos);
    }
    if (!entry.empty()) {
      auto parsed_entry = TryMakeBaggageEntry(entry);
      if (parsed_entry) {
        entries_.emplace_back(std::move(*parsed_entry));
      } else {
        is_valid_header_ = false;
      }
    }
    if (header_pos != std::string::npos) {
      header_pos++;
    }
  }
}

std::optional<BaggageEntry> Baggage::TryMakeBaggageEntry(
    std::string_view entry) {
  if (entry.find(',') != std::string_view::npos) {
    LOG_LIMITED_WARNING() << "Entry contains invalid symbol: ','";
    return std::nullopt;
  }
  if (std::find_if(entry.begin(), entry.end(), [](unsigned char x) {
        return std::isspace(x);
      }) != entry.end()) {
    LOG_LIMITED_WARNING() << "Entry contains spaces";
    return std::nullopt;
  }

  std::string_view key{entry};
  auto entry_delimiter = entry.find('=');
  if (entry_delimiter == std::string::npos) {
    LOG_LIMITED_WARNING()
        << "Can't find '=' between key and value in Baggage header. Entry: "
        << std::string(entry);
    return std::nullopt;
  }
  key.remove_suffix(entry.size() - entry_delimiter);
  if (!allowed_keys_.count(http::parser::UrlDecode(key))) {
    LOG_LIMITED_WARNING() << fmt::format("Key {} is not available", key);
    return std::nullopt;
  }

  std::string_view value{entry};
  value.remove_prefix(entry_delimiter + 1);
  auto property_pos = entry.find(';', entry_delimiter);
  if (value.empty() || key.empty()) {
    LOG_LIMITED_WARNING() << "Empty key or value in Baggage header";
    return std::nullopt;
  }
  if (property_pos == std::string_view::npos) {
    if (value.find('=') != std::string_view::npos) {
      LOG_LIMITED_WARNING() << "Entry value contains invalid symbol: '='";
      return std::nullopt;
    }
    return BaggageEntry(key, value, {});
  }
  value.remove_suffix(entry.size() - property_pos);
  if (value.empty()) {
    LOG_LIMITED_WARNING() << "Empty value in Baggage header";
    return std::nullopt;
  }
  if (value.find('=') != std::string_view::npos) {
    LOG_LIMITED_WARNING() << "Entry value contains invalid symbol: '='";
    return std::nullopt;
  }

  // make properties
  std::vector<BaggageEntryProperty> properties;
  properties.reserve(kEntitiesLimit);
  while (property_pos != std::string_view::npos) {
    std::string_view property{entry};
    property.remove_prefix(property_pos + 1);
    property_pos = entry.find(';', property_pos + 1);
    if (property_pos != std::string_view::npos) {
      property.remove_suffix(entry.size() - property_pos);
    }
    if (!property.empty()) {
      auto parsed_property = TryMakeBaggageEntryProperty(property);
      if (parsed_property) {
        properties.push_back(std::move(*parsed_property));
      } else {
        is_valid_header_ = false;
      }
    }
  }
  return std::make_optional<BaggageEntry>({key, value, properties});
}

std::optional<BaggageEntryProperty> Baggage::TryMakeBaggageEntryProperty(
    std::string_view property) {
  if (property.empty()) {
    LOG_LIMITED_WARNING() << "Empty property in Baggage header";
    return std::nullopt;
  }

  if (std::find_if(property.begin(), property.end(), [](unsigned char x) {
        return std::isspace(x);
      }) != property.end()) {
    LOG_LIMITED_WARNING() << "Property contains spaces";
    return std::nullopt;
  }

  if (property.find(',') != std::string_view::npos ||
      property.find(';') != std::string_view::npos) {
    LOG_LIMITED_WARNING() << "Property contains invalid symbol";
    return std::nullopt;
  }

  auto property_delimiter = property.find('=');
  if (property_delimiter == std::string_view::npos) {
    return std::make_optional<BaggageEntryProperty>({property, std::nullopt});
  }
  std::string_view key{property};
  key.remove_suffix(property.size() - property_delimiter);
  std::string_view value{property};
  value.remove_prefix(property_delimiter + 1);
  if (value.find('=') != std::string_view::npos) {
    LOG_LIMITED_WARNING() << "Property contains invalid symbol";
    return std::nullopt;
  }
  if (value.empty() || key.empty()) {
    LOG_LIMITED_WARNING() << "Empty key or value in Baggage header";
    return std::nullopt;
  }
  return std::make_optional<BaggageEntryProperty>(
      {key, std::make_optional<std::string_view>(std::move(value))});
}

std::optional<Baggage> TryMakeBaggage(
    std::string header, std::unordered_set<std::string> allowed_keys) {
  if (header.size() > kHeaderLengthLimit) {
    LOG_LIMITED_WARNING() << fmt::format(
        "Exceeded the limit of header length: {}", kHeaderLengthLimit);
    return std::nullopt;
  }

  return std::make_optional<Baggage>(
      {std::move(header), std::move(allowed_keys)});
}

}  // namespace baggage

USERVER_NAMESPACE_END
