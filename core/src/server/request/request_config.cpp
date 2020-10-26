#include "request_config.hpp"

#include <stdexcept>

namespace server::request {
namespace {

const std::string kHttp = "http";

RequestConfig::Type StringToType(const std::string& str) {
  if (str == kHttp) return RequestConfig::Type::kHttp;
  throw std::runtime_error("unknown RequestConfig Type: '" + str + '\'');
}

}  // namespace

RequestConfig::RequestConfig(yaml_config::YamlConfig value)
    : yaml_config::YamlConfig(std::move(value)),
      type_(StringToType(value["type"].As<std::string>(kHttp))) {}

const RequestConfig::Type& RequestConfig::GetType() const { return type_; }

const std::string& RequestConfig::TypeToString(Type type) {
  switch (type) {
    case Type::kHttp:
      return kHttp;
  }
  throw std::runtime_error(
      "can't convert to string unknown RequestConfig::Type (" +
      std::to_string(static_cast<int>(type)) + ')');
}

RequestConfig Parse(const yaml_config::YamlConfig& value,
                    formats::parse::To<RequestConfig>) {
  return RequestConfig(value);
}

}  // namespace server::request
