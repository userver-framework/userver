#pragma once

#include <string>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>

#include <storages/mysql/settings/auth_settings.hpp>
#include <storages/mysql/settings/pool_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::settings {

class HostSettings final {
 public:
  HostSettings(clients::dns::Resolver& resolver, std::string hostname,
               std::uint32_t port);

  std::string GetHostIp(userver::engine::Deadline deadline) const;

  const AuthSettings& GetAuthSettings() const;

  std::uint32_t GetPort() const;

  const PoolSettings& GetPoolSettings() const;

 private:
  clients::dns::Resolver& resolver_;

  std::string hostname_{};
  AuthSettings auth_settings_{};
  PoolSettings pool_settings_{};
  uint32_t port_{};
};

}  // namespace storages::mysql::settings

USERVER_NAMESPACE_END
