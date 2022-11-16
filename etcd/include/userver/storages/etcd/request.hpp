#pragma once

#include <memory>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

class Component {
 public:
  std::pair<std::string, std::string> KeyValue() const;
  std::string GetKey() const;
  std::string GetValue() const;

 private:
  std::string key_;
  std::string value_;
};

class Request {
 public:
  std::vector<Component> Get() const;

 private:
  std::vector<Component> components_;
};

}  // namespace storages::etcd

USERVER_NAMESPACE_END