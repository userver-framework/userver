#pragma once

#include <cstdint>
#include <optional>

#include <userver/crypto/certificate.hpp>
#include <userver/crypto/private_key.hpp>
#include <userver/server/request/request_config.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include "connection_config.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::net {

struct ListenerConfig {
  ConnectionConfig connection_config;
  request::HttpRequestConfig handler_defaults;
  std::string unix_socket_path;
  uint16_t port = 0;
  int backlog = 1024;  // truncated to net.core.somaxconn
  size_t max_connections = 32768;
  std::optional<size_t> shards;
  std::string task_processor;

  bool tls{false};
  crypto::Certificate tls_cert;
  std::string tls_private_key_path;
  std::string tls_private_key_passphrase_name;
  crypto::PrivateKey tls_private_key;
  std::vector<crypto::Certificate> tls_certificate_authorities;
};

ListenerConfig Parse(const yaml_config::YamlConfig& value,
                     formats::parse::To<ListenerConfig>);

}  // namespace server::net

USERVER_NAMESPACE_END
