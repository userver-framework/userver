#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <optional>

#include "response.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

class Client {
 public:
  virtual ~Client() = default;

  virtual Response Get(const std::string& key_begin,
                       const std::optional<std::string>& key_end = std::nullopt) const = 0;

  virtual Response GetByPrefix(const std::string& prefix) const = 0;

  virtual void Put(const std::string& key, const std::string& value) const = 0;

  virtual void Delete(const std::string& key_begin,
                      const std::optional<std::string>& key_end = std::nullopt) const = 0;

  virtual void DeleteByPrefix(const std::string& key) const = 0;

  virtual void SetWatchCallback(std::function<void(bool, std::string, std::string)>) = 0;

  virtual bool IsCallbackSet() = 0;
};

}  // namespace storages::etcd

USERVER_NAMESPACE_END
