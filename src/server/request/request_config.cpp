#include "request_config.hpp"

#include <stdexcept>

#include <yandex/taxi/userver/json_config/value.hpp>

namespace server {
namespace request {
namespace {

const std::string kHttp = "http";

RequestConfig::Type StringToType(const std::string& str) {
  if (str == kHttp) return RequestConfig::Type::kHttp;
  throw std::runtime_error("unknown RequestConfig Type: '" + str + '\'');
}

}  // namespace

RequestConfig::RequestConfig(Json::Value json, std::string full_path,
                             json_config::VariableMapPtr config_vars_ptr)
    : json_config::JsonConfig(std::move(json), std::move(full_path),
                              std::move(config_vars_ptr)) {}

const RequestConfig::Type& RequestConfig::GetType() const { return type_; }

RequestConfig RequestConfig::ParseFromJson(
    const Json::Value& json, const std::string& full_path,
    const json_config::VariableMapPtr& config_vars_ptr) {
  RequestConfig config(json, full_path, config_vars_ptr);
  auto type = json_config::ParseOptionalString(json, "type", full_path,
                                               config_vars_ptr);
  if (type) config.type_ = StringToType(*type);
  return config;
}

const std::string& RequestConfig::TypeToString(Type type) {
  switch (type) {
    case Type::kHttp:
      return kHttp;
  }
  throw std::runtime_error(
      "can't convert to string unknown RequestConfig::Type (" +
      std::to_string(static_cast<int>(type)) + ')');
}

}  // namespace request
}  // namespace server
