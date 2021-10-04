#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include <userver/clients/dns/common.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>

namespace clients::dns {

class NetResolver {
 public:
  struct Config {
    std::chrono::milliseconds query_timeout{std::chrono::seconds{1}};
    int attempts{1};
    std::vector<std::string> servers;
  };

  struct Response {
    AddrVector addrs;
    std::chrono::system_clock::time_point received_at;
    std::chrono::seconds ttl{0};
  };

  // blocking, reads resolv.conf for nameservers and some of the options
  NetResolver(engine::TaskProcessor& fs_task_processor, const Config&);
  ~NetResolver();

  NetResolver(const NetResolver&) = delete;
  NetResolver(NetResolver&&) = delete;

  engine::Future<Response> Resolve(
      std::string name,
      engine::io::AddrDomain domain = engine::io::AddrDomain::kUnspecified);

 private:
  class Impl;
  constexpr static size_t kSize = 752;
  constexpr static size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment> impl_;
};

}  // namespace clients::dns
