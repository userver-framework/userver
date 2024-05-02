#pragma once

#include <map>

#include <userver/components/component_config.hpp>
#include <userver/formats/json/value.hpp>

#include <kafka/impl/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace cppkafka {
class Configuration;
}  // namespace cppkafka

namespace kafka {

struct Secret {
  std::string username;
  std::string password;
  std::string brokers;
};

class BrokerSecrets {
 public:
  BrokerSecrets(const formats::json::Value& doc);

  const Secret& GetSecretByComponentName(
      const std::string& component_name) const;

 private:
  Secret Parse(const formats::json::Value& doc, formats::parse::To<Secret>);

 private:
  std::map<std::string, Secret> secrets_;
};

std::unique_ptr<cppkafka::Configuration> SetErrorCallback(
    std::unique_ptr<cppkafka::Configuration> config, impl::Stats& stats);

std::unique_ptr<cppkafka::Configuration> MakeConsumerConfiguration(
    const BrokerSecrets& secrets, const components::ComponentConfig& config);

}  // namespace kafka

USERVER_NAMESPACE_END
