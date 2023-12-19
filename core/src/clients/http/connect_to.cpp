#include <userver/clients/http/connect_to.hpp>

#include <curl-ev/native.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

ConnectTo::ConnectTo(ConnectTo&& other) noexcept : value_(other.value_) {
  other.value_ = nullptr;
}

ConnectTo::ConnectTo(const std::string& value) {
  if (!value.empty()) {
    value_ = curl::native::curl_slist_append(nullptr, value.data());
  }
}

ConnectTo::~ConnectTo() {
  if (value_) {
    curl::native::curl_slist_free_all(value_);
  }
}

ConnectTo& ConnectTo::operator=(ConnectTo&& other) noexcept {
  std::swap(other.value_, value_);
  return *this;
}

curl::native::curl_slist* ConnectTo::GetUnderlying() const noexcept {
  return value_;
}

}  // namespace clients::http

USERVER_NAMESPACE_END
