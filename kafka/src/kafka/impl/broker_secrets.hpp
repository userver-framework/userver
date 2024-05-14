#pragma once

#include <map>

#include <userver/formats/json/value.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

struct Secret final {
  using Password = utils::NonLoggable<class PasswordTag, std::string>;

  std::string brokers;
  std::string username;
  Password password;
};

class BrokerSecrets final {
 public:
  explicit BrokerSecrets(const formats::json::Value& doc);

  const Secret& GetSecretByComponentName(
      const std::string& component_name) const;

 private:
  std::map<std::string, Secret> secret_by_component_name_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
