#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

#include <userver/clients/dns/common.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {

class FileResolver {
 public:
  FileResolver(engine::TaskProcessor& fs_task_processor, std::string path,
               std::chrono::milliseconds update_interval);

  AddrVector Resolve(const std::string& name) const;

  void ReloadHosts();

 private:
  engine::TaskProcessor& fs_task_processor_;
  const std::string path_;
  rcu::Variable<std::unordered_map<std::string, AddrVector>> hosts_;
  utils::PeriodicTask update_task_;
};

}  // namespace clients::dns

USERVER_NAMESPACE_END
