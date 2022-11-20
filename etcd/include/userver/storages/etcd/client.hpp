#pragma once

#include <chrono>
#include <memory>
#include <string>

#include <userver/storages/etcd/messages.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

class Client {
 public:
  virtual ~Client() = default;

  virtual Message GetRange(const std::string& key_begin,
                           const std::string& key_end) const = 0;

  virtual void Put(const std::string& key, const std::string& value) const = 0;

  virtual void Delete(const std::string& key) const = 0;
};

}  // namespace storages::etcd

USERVER_NAMESPACE_END
