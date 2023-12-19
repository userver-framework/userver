#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include <userver/clients/dns/common.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {

class NetResolver {
 public:
  struct Response {
    AddrVector addrs;
    std::chrono::system_clock::time_point received_at;
    std::chrono::seconds ttl{0};
  };

  // reads resolv.conf for nameservers and some of the options from FS-TP
  NetResolver(engine::TaskProcessor& fs_task_processor,
              std::chrono::milliseconds query_timeout, int query_attempts,
              const std::vector<std::string>& custom_servers = {});
  ~NetResolver();

  NetResolver(const NetResolver&) = delete;
  NetResolver(NetResolver&&) = delete;

  engine::Future<Response> Resolve(std::string name);

 private:
  class Impl;
  const std::unique_ptr<Impl> impl_;
};

}  // namespace clients::dns

USERVER_NAMESPACE_END
