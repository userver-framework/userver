#pragma once

#include <memory>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

class Component {
 public:
  std::pair<std::string, std::string> KeyValue();
  std::string GetKey();
  std::string GetValue();

 private:
  std::string key_;
  std::string value_;
};

class Request {
 public:
  std::vector<Component> Get();

 private:
  std::vector<Component> components_;
};

}  // namespace storages::etcd

USERVER_NAMESPACE_END