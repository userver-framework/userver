#pragma once

#include <string>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {
namespace request {

class RequestConfig final : public yaml_config::YamlConfig {
 public:
  enum class Type { kHttp };

  explicit RequestConfig(yaml_config::YamlConfig value);

  const Type& GetType() const;

  static const std::string& TypeToString(Type type);

 private:
  Type type_ = Type::kHttp;
};

RequestConfig Parse(const yaml_config::YamlConfig& value,
                    formats::parse::To<RequestConfig>);

}  // namespace request
}  // namespace server

USERVER_NAMESPACE_END
