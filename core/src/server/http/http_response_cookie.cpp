#include <server/http/http_response_cookie.hpp>

#include <sstream>

#include <utils/datetime.hpp>

namespace {
const std::string kTimeFormat = "%a, %d %b %Y %H:%M:%S %Z";
}

namespace server::http {

/// CookieData class

class Cookie::CookieData final {
 public:
  CookieData() = default;
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

  [[nodiscard]] std::string ToString() const;

 private:
  std::string name_;
  std::string value_;
  std::string path_;
  std::string domain_;

  bool secure_{false};
  bool http_only_{false};
  std::chrono::system_clock::time_point expires_{};
  std::chrono::seconds max_age_{0};
};

Cookie::CookieData::CookieData(std::string&& name, std::string&& value)
    : name_(std::move(name)), value_(std::move(value)) {}

const std::string& Cookie::CookieData::Name() const { return name_; }
const std::string& Cookie::CookieData::Value() const { return value_; }

bool Cookie::CookieData::IsSecure() const { return secure_; }
void Cookie::CookieData::SetSecure() { secure_ = true; }

std::chrono::system_clock::time_point Cookie::CookieData::Expires() const {
  return expires_;
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

std::chrono::seconds Cookie::CookieData::MaxAge() const { return max_age_; }
void Cookie::CookieData::SetMaxAge(std::chrono::seconds value) {
  max_age_ = value;
}

std::string Cookie::CookieData::ToString() const {
  std::stringstream ss;
  ss << name_ << '=' << value_;
  if (!domain_.empty()) ss << "; Domain=" << domain_;
  if (!path_.empty()) ss << "; Path=" << path_;
  if (expires_ > std::chrono::system_clock::time_point{})
    ss << "; Expires="
       << utils::datetime::Timestring(expires_, "GMT", kTimeFormat);
  if (max_age_ > std::chrono::seconds{0})
    ss << "; Max-Age=" << max_age_.count();
  if (secure_) ss << "; Secure";
  if (http_only_) ss << "; HttpOnly";
  return ss.str();
}

// Cookie class

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

std::string Cookie::ToString() const { return data_->ToString(); }

}  // namespace server::http
