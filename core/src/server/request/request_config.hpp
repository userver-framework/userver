#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

struct HttpRequestConfig {
  std::size_t max_url_size = 8192;
  std::size_t max_request_size = 1024 * 1024;
  std::size_t max_headers_size = 65536;
  bool parse_args_from_body = false;
  bool testing_mode = false;
  bool decompress_request = false;
};

class RequestConfig final {
 public:
  enum class Type { kHttp };

  constexpr explicit RequestConfig(const HttpRequestConfig& config)
      : config_(config) {}

  Type GetType() const;

  const HttpRequestConfig& GetHttpConfig() const {
    return std::get<HttpRequestConfig>(config_);
  }

  static const std::string& TypeToString(Type type);

 private:
  std::variant<HttpRequestConfig> config_{};
};

RequestConfig Parse(const yaml_config::YamlConfig& value,
                    formats::parse::To<RequestConfig>);

}  // namespace server::request

USERVER_NAMESPACE_END
