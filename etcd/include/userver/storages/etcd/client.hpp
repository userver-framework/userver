#pragma once

#include <chrono>
#include <memory>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

class Client {
 public:
  virtual ~Client() = default;
  void Remove(const std::string& key);
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
