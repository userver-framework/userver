#pragma once

#include <memory>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

class KeyValue {
 public:
  KeyValue(const std::string& key, const std::string& value);

  std::pair<std::string, std::string> KeyValue() const;

  std::string GetKey() const;

  std::string GetValue() const;

 private:
  std::string key_;

  std::string value_;
};

class Message {
 public:
  Message(const std::vector<KeyValue>& key_values);

  std::vector<KeyValue> Get() const;

 private:
  std::vector<KeyValue> key_values_;
};

}  // namespace storages::etcd

USERVER_NAMESPACE_END