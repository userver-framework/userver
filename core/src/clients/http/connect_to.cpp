#include <userver/clients/http/connect_to.hpp>

#include <curl-ev/native.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

ConnectTo::ConnectTo(const std::string& value) {
  if (!value.empty()) {
    value_ = curl::native::curl_slist_append(nullptr, value.data());
  }
}

ConnectTo::~ConnectTo() noexcept {
  if (value_) {
    curl::native::curl_slist_free_all(value_);
  }
}

curl::native::curl_slist* ConnectTo::GetUnderlying() const noexcept {
  return value_;
}

}  // namespace clients::http

USERVER_NAMESPACE_END
