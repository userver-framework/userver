#include <userver/server/http/http_response_cookie.hpp>

#include <array>
#include <boost/algorithm/string.hpp>

#include <fmt/compile.h>
#include <unordered_map>
#include <userver/logging/log.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
const std::string kTimeFormat = "%a, %d %b %Y %H:%M:%S %Z";
constexpr std::string_view kDomain = "Domain";
constexpr std::string_view kExpires = "Expires";
constexpr std::string_view kHttpOnly = "HttpOnly";
constexpr std::string_view kMaxAge = "Max-Age";
constexpr std::string_view kPath = "Path";
constexpr std::string_view kSecure = "Secure";
constexpr std::string_view kSameSite = "SameSite";

enum class KeyType {
  kDomain,
  kExpires,
  kHttpOnly,
  kMaxAge,
  kPath,
  kSecure,
  kUnknown,
  kSameSite,
};

KeyType GetKeyType(std::string_view type) {
  static const std::unordered_map<std::string_view, KeyType,
                                  utils::StrIcaseHash, utils::StrIcaseEqual>
      kCookiesKeyStringToType{
          {kDomain, KeyType::kDomain},     {kExpires, KeyType::kExpires},
          {kHttpOnly, KeyType::kHttpOnly}, {kMaxAge, KeyType::kMaxAge},
          {kPath, KeyType::kPath},         {kSecure, KeyType::kSecure},
          {kSameSite, KeyType::kSameSite}};
  auto it = kCookiesKeyStringToType.find(type);
  if (it == kCookiesKeyStringToType.end()) {
    return KeyType::kUnknown;
  }
  return it->second;
}

std::optional<std::chrono::system_clock::time_point> ParseTime(
    const std::string& timestring) {
  if (timestring.size() < 3) {
    return std::nullopt;
  }
  try {
    return utils::datetime::Stringtime(
        timestring, timestring.substr(timestring.size() - 3), kTimeFormat);
  } catch (const utils::datetime::DateParseError& err) {
    LOG_WARNING() << "Error while parsing cookie time: " << err.what();
    return std::nullopt;
  } catch (const utils::datetime::TimezoneLookupError& err) {
    LOG_WARNING() << "Error while parsing cookie timezone: " << err.what();
    return std::nullopt;
  }
}

struct CookieKeyValue {
  size_t shift;
  std::string key;
  std::optional<std::string> value{};
};

std::optional<CookieKeyValue> GetCookieKeyValueOpt(const char* start,
                                                   size_t size) {
  if (!size) return std::nullopt;
  const char* ptr = start;
  const char* end = ptr + size;
  while (ptr != end && *ptr == ' ') ptr++;
  if (ptr == end) {
    return std::nullopt;
  }
  size_t size_from_ptr = size - (ptr - start);

  const char* equal_pos =
      static_cast<const char*>(memchr(ptr, '=', size_from_ptr));
  if (equal_pos + 1 == end) {
    return {
        {static_cast<size_t>(end - start), std::string(ptr, equal_pos - ptr)}};
  }

  const char* end_pair_pos =
      static_cast<const char*>(memchr(ptr, ';', size_from_ptr));

  if (equal_pos == nullptr) {
    if (end_pair_pos == nullptr) {
      return {{size, std::string(ptr, size_from_ptr)}};
    }
    return {{static_cast<size_t>(end_pair_pos - start) + 1,
             std::string(ptr, end_pair_pos - ptr)}};
  }
  if (end_pair_pos != nullptr && end_pair_pos < equal_pos) {
    return {{static_cast<size_t>(end_pair_pos - start) + 1,
             std::string(ptr, end_pair_pos - ptr)}};
  }

  std::string key(ptr, equal_pos - ptr);

  size_t shift = size;
  if (end_pair_pos == nullptr) {
    end_pair_pos = end;
  } else {
    shift = static_cast<size_t>(end_pair_pos - start) + 1;
  }
  if (equal_pos + 1 == end_pair_pos) {
    return {{static_cast<size_t>(end_pair_pos - start) + 1, std::move(key)}};
  }
  equal_pos++;
  std::string value(equal_pos, end_pair_pos - equal_pos);
  return {{shift, std::move(key), std::move(value)}};
}

std::optional<server::http::Cookie> ParseSingleCookie(const char* ptr,
                                                      size_t size) {
  if (!size) return std::nullopt;
  auto cookie_opt = GetCookieKeyValueOpt(ptr, size);
  if (!cookie_opt.has_value()) {
    return std::nullopt;
  }
  ptr += cookie_opt->shift;
  size -= cookie_opt->shift;
  std::optional<server::http::Cookie> cookie;
  try {
    cookie = server::http::Cookie(
        cookie_opt->key,
        cookie_opt->value.has_value() ? std::move(*cookie_opt->value) : "");
  } catch (const std::runtime_error& parse_error) {
    LOG_WARNING() << "Cookie with name '" << cookie_opt->key
                  << "' will be skipped because of validation error: "
                  << parse_error.what();
    return std::nullopt;
  }
  while (auto cookie_options = GetCookieKeyValueOpt(ptr, size)) {
    ptr += cookie_options->shift;
    size -= cookie_options->shift;
    switch (GetKeyType(cookie_options->key)) {
      case KeyType::kDomain:
        if (cookie_options->value.has_value()) {
          cookie->SetDomain(std::move(*cookie_options->value));
        }
        break;
      case KeyType::kPath:
        if (cookie_options->value.has_value()) {
          cookie->SetPath(std::move(*cookie_options->value));
        }
        break;
      case KeyType::kMaxAge:
        if (cookie_options->value.has_value()) {
          cookie->SetMaxAge(
              std::chrono::seconds{std::stoll(*cookie_options->value)});
        }
        break;
      case KeyType::kExpires:
        if (cookie_options->value.has_value()) {
          if (auto datetime = ParseTime(std::move(*cookie_options->value))) {
            cookie->SetExpires(datetime.value());
          }
        }
        break;
      case KeyType::kSameSite:
        if (cookie_options->value.has_value()) {
          cookie->SetSameSite(std::move(*cookie_options->value));
        }
        break;
      case KeyType::kHttpOnly:
        cookie->SetHttpOnly();
        break;
      case KeyType::kSecure:
        cookie->SetSecure();
        break;
      case KeyType::kUnknown:
        break;
    }
  }
  return cookie;
}

}  // namespace

namespace server::http {

/// CookieData class

class Cookie::CookieData final {
 public:
  CookieData(std::string&& name, std::string&& value);
  CookieData(const CookieData&) = default;
  ~CookieData() = default;

  [[nodiscard]] const std::string& Name() const;
  [[nodiscard]] const std::string& Value() const;

  [[nodiscard]] bool IsSecure() const;
  void SetSecure();

  [[nodiscard]] std::chrono::system_clock::time_point Expires() const;
  void SetExpires(std::chrono::system_clock::time_point value);

  [[nodiscard]] bool IsPermanent() const;
  void SetPermanent();

  [[nodiscard]] bool IsHttpOnly() const;
  void SetHttpOnly();

  [[nodiscard]] const std::string& Path() const;
  void SetPath(std::string&& value);

  [[nodiscard]] const std::string& Domain() const;
  void SetDomain(std::string&& value);

  [[nodiscard]] std::chrono::seconds MaxAge() const;
  void SetMaxAge(std::chrono::seconds value);

  [[nodiscard]] std::string SameSite() const;
  void SetSameSite(std::string value);

  void AppendToString(std::string& os) const;

 private:
  void ValidateName() const;
  void ValidateValue() const;

  std::string name_;
  std::string value_;
  std::string path_;
  std::string domain_;
  std::string same_site_;

  bool secure_{false};
  bool http_only_{false};
  std::optional<std::chrono::system_clock::time_point> expires_{};
  std::optional<std::chrono::seconds> max_age_{};
};

Cookie::CookieData::CookieData(std::string&& name, std::string&& value)
    : name_(std::move(name)), value_(std::move(value)) {
  ValidateName();
  ValidateValue();
}

const std::string& Cookie::CookieData::Name() const { return name_; }
const std::string& Cookie::CookieData::Value() const { return value_; }

bool Cookie::CookieData::IsSecure() const { return secure_; }
void Cookie::CookieData::SetSecure() { secure_ = true; }

std::chrono::system_clock::time_point Cookie::CookieData::Expires() const {
  return expires_.value_or(std::chrono::system_clock::time_point{});
}
void Cookie::CookieData::SetExpires(
    std::chrono::system_clock::time_point value) {
  expires_ = value;
}

bool Cookie::CookieData::IsPermanent() const {
  return expires_ == std::chrono::system_clock::time_point::max();
}
void Cookie::CookieData::SetPermanent() {
  expires_ = std::chrono::system_clock::time_point::max();
}

bool Cookie::CookieData::IsHttpOnly() const { return http_only_; }
void Cookie::CookieData::SetHttpOnly() { http_only_ = true; }

const std::string& Cookie::CookieData::Path() const { return path_; }
void Cookie::CookieData::SetPath(std::string&& value) {
  path_ = std::move(value);
}

const std::string& Cookie::CookieData::Domain() const { return domain_; }
void Cookie::CookieData::SetDomain(std::string&& value) {
  domain_ = std::move(value);
}

std::chrono::seconds Cookie::CookieData::MaxAge() const {
  return max_age_.value_or(std::chrono::seconds{0});
}
void Cookie::CookieData::SetMaxAge(std::chrono::seconds value) {
  max_age_ = value;
}

std::string Cookie::CookieData::SameSite() const { return same_site_; }
void Cookie::CookieData::SetSameSite(std::string value) {
  same_site_ = std::move(value);
}

void Cookie::CookieData::AppendToString(std::string& os) const {
  os.append(name_);
  os.append("=");
  os.append(value_);

  if (!domain_.empty()) {
    os.append("; Domain=");
    os.append(domain_);
  }
  if (!path_.empty()) {
    os.append("; Path=");
    os.append(path_);
  }
  if (expires_.has_value()) {
    os.append("; Expires=");
    os.append(
        utils::datetime::Timestring(expires_.value(), "GMT", kTimeFormat));
  }
  if (max_age_.has_value()) {
    os.append("; Max-Age=");
    fmt::format_to(std::back_inserter(os), FMT_COMPILE("{}"),
                   max_age_.value().count());
  }
  if (secure_) {
    os.append("; Secure");
  }
  if (!same_site_.empty()) {
    os.append("; SameSite=");
    os.append(same_site_);
  }
  if (http_only_) {
    os.append("; HttpOnly");
  }
}

void Cookie::CookieData::ValidateName() const {
  static constexpr auto kBadNameChars = []() {
    std::array<bool, 256> res{};  // Zero initializes
    for (int i = 0; i < 32; i++) res[i] = true;
    for (int i = 127; i < 256; i++) res[i] = true;
    for (unsigned char c : "()<>@,;:\\\"/[]?={} \t") res[c] = true;
    return res;
  }();

  if (name_.empty()) throw std::runtime_error("Empty cookie name");

  for (char c : name_) {
    auto code = static_cast<uint8_t>(c);
    if (kBadNameChars[code]) {
      throw std::runtime_error(
          fmt::format("Invalid character in cookie name: '{}' (#{})", c, code));
    }
  }
}

void Cookie::CookieData::ValidateValue() const {
  // `cookie-value` from https://tools.ietf.org/html/rfc6265#section-4.1.1
  static constexpr auto kBadValueChars = []() {
    std::array<bool, 256> res{};  // Zero initializes
    for (int i = 0; i <= 32; i++) res[i] = true;
    for (int i = 127; i < 256; i++) res[i] = true;
    res[0x22] = true;  // `"`
    res[0x2C] = true;  // `,`
    res[0x3B] = true;  // `;`
    res[0x5C] = true;  // `\`
    return res;
  }();

  std::string_view value(value_);
  if (value.size() > 1 && value.front() == '"' && value.back() == '"')
    value = value.substr(1, value.size() - 2);

  for (char c : value) {
    auto code = static_cast<uint8_t>(c);
    if (kBadValueChars[code]) {
      throw std::runtime_error(fmt::format(
          "Invalid character in cookie value: '{}' (#{})", c, code));
    }
  }
}

// Cookie class

/// static
std::optional<Cookie> Cookie::FromString(std::string_view cookie) {
  return ParseSingleCookie(cookie.data(), cookie.size());
}

Cookie::Cookie(std::string name, std::string value)
    : data_(std::make_unique<CookieData>(std::move(name), std::move(value))) {}
Cookie::Cookie(Cookie&& cookie) noexcept = default;
Cookie::~Cookie() noexcept = default;
Cookie::Cookie(const Cookie& cookie) { *this = cookie; }

Cookie& Cookie::operator=(Cookie&&) noexcept = default;
Cookie& Cookie::operator=(const Cookie& cookie) {
  if (this == &cookie) return *this;

  data_ = std::make_unique<CookieData>(*cookie.data_);
  return *this;
}

const std::string& Cookie::Name() const { return data_->Name(); }
const std::string& Cookie::Value() const { return data_->Value(); }

bool Cookie::IsSecure() const { return data_->IsSecure(); }
Cookie& Cookie::SetSecure() {
  data_->SetSecure();
  return *this;
}

std::chrono::system_clock::time_point Cookie::Expires() const {
  return data_->Expires();
}
Cookie& Cookie::SetExpires(std::chrono::system_clock::time_point value) {
  data_->SetExpires(value);
  return *this;
}

bool Cookie::IsPermanent() const { return data_->IsPermanent(); }
Cookie& Cookie::SetPermanent() {
  data_->SetPermanent();
  return *this;
}

bool Cookie::IsHttpOnly() const { return data_->IsHttpOnly(); }
Cookie& Cookie::SetHttpOnly() {
  data_->SetHttpOnly();
  return *this;
}

const std::string& Cookie::Path() const { return data_->Path(); }
Cookie& Cookie::SetPath(std::string value) {
  data_->SetPath(std::move(value));
  return *this;
}

const std::string& Cookie::Domain() const { return data_->Domain(); }
Cookie& Cookie::SetDomain(std::string value) {
  data_->SetDomain(std::move(value));
  return *this;
}

std::chrono::seconds Cookie::MaxAge() const { return data_->MaxAge(); }
Cookie& Cookie::SetMaxAge(std::chrono::seconds value) {
  data_->SetMaxAge(value);
  return *this;
}

std::string Cookie::SameSite() const { return data_->SameSite(); }
Cookie& Cookie::SetSameSite(std::string value) {
  data_->SetSameSite(std::move(value));
  return *this;
}

std::string Cookie::ToString() const {
  std::string os;
  data_->AppendToString(os);
  return os;
}

void Cookie::AppendToString(std::string& os) const {
  data_->AppendToString(os);
}

}  // namespace server::http

USERVER_NAMESPACE_END
