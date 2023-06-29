#pragma once

/// @file userver/clients/http/connect_to.hpp
/// @brief @copybrief clients::http::ConnectTo

#include <string>

USERVER_NAMESPACE_BEGIN

namespace curl::native {
struct curl_slist;
}

namespace clients::http {

/// @brief CURLOPT_CONNECT_TO argument for curl's connect_to().
///
/// @warning ConnectTo passed to connect_to() must outlive http's Request as
/// it holds curl's slist value.
class ConnectTo final {
 public:
  ConnectTo(ConnectTo&&) noexcept;

  ConnectTo(const ConnectTo&) = delete;

  explicit ConnectTo(const std::string& value);

  ~ConnectTo();

  ConnectTo& operator=(const ConnectTo&) = delete;

  ConnectTo& operator=(ConnectTo&&) noexcept;

  curl::native::curl_slist* GetUnderlying() const noexcept;

 private:
  curl::native::curl_slist* value_{nullptr};
};

}  // namespace clients::http

USERVER_NAMESPACE_END
