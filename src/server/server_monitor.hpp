#pragma once

#include <string>

#include <server/request/monitor.hpp>
#include <server/request/request_base.hpp>

namespace server {

class ServerImpl;

class ServerMonitor : public request::Monitor {
 public:
  explicit ServerMonitor(const ServerImpl& server_impl);

  std::string GetJsonData(const request::RequestBase&) const override;

 private:
  const ServerImpl& server_impl_;
};

}  // namespace server
