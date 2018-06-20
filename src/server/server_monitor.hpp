#pragma once

#include <string>

#include <server/http/http_request.hpp>

namespace server {

class ServerImpl;

class ServerMonitor {
 public:
  explicit ServerMonitor(const ServerImpl& server_impl);

  std::string GetJsonData(const http::HttpRequest&) const;

 private:
  const ServerImpl& server_impl_;
};

}  // namespace server
