#include <userver/server/request/request_config.hpp>

#include <stdexcept>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

HttpRequestConfig Parse(const yaml_config::YamlConfig& value,
                        formats::parse::To<HttpRequestConfig>) {
  HttpRequestConfig conf{};

  conf.max_url_size = value["max_url_size"].As<size_t>(conf.max_url_size);

  conf.max_request_size =
      value["max_request_size"].As<size_t>(conf.max_request_size);

  conf.max_headers_size =
      value["max_headers_size"].As<size_t>(conf.max_headers_size);

  conf.parse_args_from_body =
      value["parse_args_from_body"].As<bool>(conf.parse_args_from_body);

  conf.set_tracing_headers =
      value["set_tracing_headers"].As<bool>(conf.set_tracing_headers);

  return conf;
}

}  // namespace server::request

USERVER_NAMESPACE_END
