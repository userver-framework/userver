#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace curl::native {
struct curl_slist;
}

namespace clients::http {

class ConnectTo final {
 public:
  ConnectTo(const std::string& value);
  ~ConnectTo() noexcept;

  curl::native::curl_slist* GetUnderlying() const noexcept;

 private:
  curl::native::curl_slist* value_{nullptr};
};

}  // namespace clients::http

USERVER_NAMESPACE_END
