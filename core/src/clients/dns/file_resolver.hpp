#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

#include <userver/clients/dns/common.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/periodic_task.hpp>

namespace clients::dns {

class FileResolver {
 public:
  struct Config {
    engine::TaskProcessor& fs_task_processor;
    std::string path;
    std::chrono::milliseconds update_interval;
  };

  explicit FileResolver(Config);

  AddrVector Resolve(const std::string& name,
                     engine::io::AddrDomain domain =
                         engine::io::AddrDomain::kUnspecified) const;

  void ReloadHosts();

 private:
  const Config config_;
  rcu::Variable<std::unordered_map<std::string, AddrVector>> hosts_;
  utils::PeriodicTask update_task_;
};

}  // namespace clients::dns
