#include <storages/mysql/settings/host_settings.hpp>

#include <fmt/format.h>

#include <userver/clients/dns/resolver.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::settings {

HostSettings::HostSettings(clients::dns::Resolver& resolver,
                           std::string hostname, uint32_t port)
    : resolver_{resolver}, hostname_{std::move(hostname)}, port_{port} {}

const AuthSettings& HostSettings::GetAuthSettings() const {
  return auth_settings_;
}

std::uint32_t HostSettings::GetPort() const { return port_; }

const PoolSettings& HostSettings::GetPoolSettings() const {
  return pool_settings_;
}

std::string HostSettings::GetHostIp(userver::engine::Deadline deadline) const {
  const auto addrs = resolver_.Resolve(hostname_, deadline);
  if (addrs.empty()) {
    throw std::runtime_error{"failed to resolve hostname"};
  }

  for (const auto& addr : addrs) {
    if (addr.Domain() == engine::io::AddrDomain::kInet) {
      return addr.PrimaryAddressString();
    }
  }

  throw std::runtime_error{"Failed to resolve any ipv4 addresses"};
}

}  // namespace storages::mysql::settings

USERVER_NAMESPACE_END
