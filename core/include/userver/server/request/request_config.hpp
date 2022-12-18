#pragma once

#include <cstdint>

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
  bool set_tracing_headers = true;
};

HttpRequestConfig Parse(const yaml_config::YamlConfig& value,
                        formats::parse::To<HttpRequestConfig>);

}  // namespace server::request

USERVER_NAMESPACE_END
