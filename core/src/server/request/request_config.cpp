#include "request_config.hpp"

#include <stdexcept>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {
namespace {

const std::string kHttp = "http";

RequestConfig::Type StringToType(const std::string& str) {
  if (str == kHttp) return RequestConfig::Type::kHttp;
  throw std::runtime_error("unknown RequestConfig Type: '" + str + '\'');
}

}  // namespace

RequestConfig::Type RequestConfig::GetType() const {
  return std::visit([](const HttpRequestConfig&) { return Type::kHttp; },
                    config_);
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

RequestConfig Parse(const yaml_config::YamlConfig& value,
                    formats::parse::To<RequestConfig>) {
  const auto type = StringToType(value["type"].As<std::string>(kHttp));

  UASSERT(type == RequestConfig::Type::kHttp);

  HttpRequestConfig conf{};

  conf.max_url_size = value["max_url_size"].As<size_t>(conf.max_url_size);

  conf.max_request_size =
      value["max_request_size"].As<size_t>(conf.max_request_size);

  conf.max_headers_size =
      value["max_headers_size"].As<size_t>(conf.max_headers_size);

  conf.parse_args_from_body =
      value["parse_args_from_body"].As<bool>(conf.parse_args_from_body);

  return RequestConfig(conf);
}

}  // namespace server::request

USERVER_NAMESPACE_END
