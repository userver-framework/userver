#include "listener_config.hpp"

#include <stdexcept>
#include <string>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/fs/blocking/read.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

ListenerConfig Parse(const yaml_config::YamlConfig& value,
                     formats::parse::To<ListenerConfig>) {
  ListenerConfig config;

  config.connection_config = value["connection"].As<ConnectionConfig>();
  config.handler_defaults =
      value["handler-defaults"].As<request::HttpRequestConfig>();
  config.port = value["port"].As<uint16_t>(0);
  config.address = value["address"].As<std::string>("::");
  config.unix_socket_path = value["unix-socket"].As<std::string>("");
  config.max_connections =
      value["max_connections"].As<size_t>(config.max_connections);
  config.shards = value["shards"].As<std::optional<size_t>>(config.shards);
  config.task_processor = value["task_processor"].As<std::string>();
  config.backlog = value["backlog"].As<int>(config.backlog);

  if (config.port != 0 && !config.unix_socket_path.empty())
    throw std::runtime_error(
        "Both 'port' and 'unix-socket' fields are set, only single field may "
        "be set at a time");
  if (config.port == 0 && config.unix_socket_path.empty())
    throw std::runtime_error(
        "Either non-zero 'port' or non-empty 'unix-socket' fields must be set");

  if (config.backlog <= 0) {
    throw std::runtime_error("Invalid backlog value in " + value.GetPath());
  }

  auto cert_path = value["tls"]["cert"].As<std::string>({});
  auto pkey_path = value["tls"]["private-key"].As<std::string>({});
  auto pkey_pass_name =
      value["tls"]["private-key-passphrase-name"].As<std::string>({});
  if (cert_path.empty() != pkey_path.empty()) {
    throw std::runtime_error(
        "Either set both tls.cert and tls.private-key options or none of them");
  }
  if (!cert_path.empty()) {
    auto contents = fs::blocking::ReadFileContents(cert_path);
    config.tls_cert = crypto::Certificate::LoadFromString(contents);
    config.tls = true;
  }
  if (!pkey_path.empty()) {
    config.tls_private_key_path = pkey_path;
  }
  if (!pkey_pass_name.empty()) {
    config.tls_private_key_passphrase_name = pkey_pass_name;
  }
  auto ca_paths = value["tls"]["ca"].As<std::vector<std::string>>({});
  for (const auto& ca_path : ca_paths) {
    auto contents = fs::blocking::ReadFileContents(ca_path);
    config.tls_certificate_authorities.push_back(
        crypto::Certificate::LoadFromString(contents));
  }

  return config;
}

}  // namespace server::net

USERVER_NAMESPACE_END
