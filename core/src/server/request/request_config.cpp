#include <userver/server/request/request_config.hpp>

#include <server/http/parse_http_status.hpp>
#include <userver/logging/level_serialization.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace {

utils::http::HttpVersion GetHttpVersion(const std::string& http_ver) {
  if (http_ver == "2") {
    return utils::http::HttpVersion::k2;
  } else if (http_ver == "1.1") {
    return utils::http::HttpVersion::k11;
  }
  throw std::runtime_error{fmt::format(
      "Invalid http protocol version. Expected '1.1' or '2', actual: "
      "{}",
      http_ver)};
}

}  // namespace

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

  conf.deadline_propagation_enabled =
      value["deadline_propagation_enabled"].As<bool>(
          conf.deadline_propagation_enabled);

  conf.deadline_expired_status_code =
      value["deadline_expired_status_code"].As<http::HttpStatus>(
          conf.deadline_expired_status_code);

  conf.http_version =
      GetHttpVersion(value["http_version"].As<std::string>("1.1"));

  return conf;
}

}  // namespace server::request

USERVER_NAMESPACE_END
