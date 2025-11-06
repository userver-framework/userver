#pragma once

/// @file userver/etcd/component.hpp
/// @brief @copybrief etcd::Component

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/etcd/client.hpp>

USERVER_NAMESPACE_BEGIN

namespace etcd {

// clang-format off
/// @ingroup userver_components
///
/// @brief Etcd client component
///
/// Provides access to a etcd cluster.
///
/// ## Static options:
/// Name                               | Description                                                | Default value
/// ---------------------------------- | ---------------------------------------------------------- | ---------------
/// endpoints                          | Etcd endpoints to which client make HTTP requests          | -
/// attempts                           | Number of attempts to each endpoint, on failed attempts client randomly moves to another endpoint | 3
/// request_timeout_ms                 | Timeout for all HTTP requests to etcd except watch request | 1000
/// watch_timeout_ms                   | Timeout for watch HTTP request. It's a stremed request, so it is used also as a connection timeout, so it should not be too short | 1000000
///
// clang-format on

class Component final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "etcd-client";

    Component(const components::ComponentConfig&, const components::ComponentContext&);

    static yaml_config::Schema GetStaticConfigSchema();

    ClientPtr GetClient();

private:
    const ClientPtr etcd_client_ptr_;
};

}  // namespace etcd

USERVER_NAMESPACE_END
