#pragma once

#include <userver/clients/dns/exception.hpp>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/components/component_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {

/// Resolver types used by the components
enum class ResolverType {
  kGetaddrinfo,  ///< resolve hosts using blocking getaddrinfo call
  kAsync,        ///< use non-blocking resolver
};

/// Returns pointer to asynchronous resolver interface if required by config
/// or nullptr for getaddrinfo
/// @throws InvalidConfigException on unknown resolver type in config
clients::dns::Resolver* GetResolverPtr(
    const components::ComponentConfig& config,
    const components::ComponentContext& context,
    ResolverType default_type = ResolverType::kAsync);

}  // namespace clients::dns

USERVER_NAMESPACE_END
