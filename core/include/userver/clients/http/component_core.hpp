#pragma once

#ifdef USERVER_TVM2_HTTP_CLIENT
#error Use components::Http from clients/http.hpp instead
#endif

/// @file userver/clients/http/component_core.hpp
/// @brief @copybrief components::HttpClientCore

#include <userver/components/component_base.hpp>
#include <userver/concurrent/async_event_source.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

class ClientCore;

}  // namespace clients::http

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that manages @ref clients::http::ClientCore.
///
/// Returned references to @ref clients::http::ClientCore live for a lifetime of the
/// component and are safe for concurrent use.
///
/// ## Dynamic options:
/// * @ref HTTP_CLIENT_CONNECT_THROTTLE
/// * @ref HTTP_CLIENT_CONNECTION_POOL_SIZE
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// pool-statistics-disable | set to true to disable statistics for connection pool | false
/// thread-name-prefix | set OS thread name to this value | ''
/// threads | number of threads to process low level HTTP related IO system calls | 8
/// fs-task-processor | task processor to run blocking HTTP related calls, like DNS resolving or hosts reading | engine::current_task::GetBlockingTaskProcessor()
/// destination-metrics-auto-max-size | set max number of automatically created destination metrics | 100
/// user-agent | User-Agent HTTP header to show on all requests, result of utils::GetUserverIdentifier() if empty | empty
/// testsuite-enabled | enable testsuite testing support | false
/// testsuite-timeout | if set, force the request timeout regardless of the value passed in code | -
/// testsuite-allowed-url-prefixes | if set, checks that all URLs start with any of the passed prefixes, asserts if not. Set for testing purposes only. | ''
/// dns_resolver | server hostname resolver type (getaddrinfo or async) | 'async'
/// set-deadline-propagation-header | whether to set http::common::kXYaTaxiClientTimeoutMs request header, see @ref scripts/docs/en/userver/deadline_propagation.md | true
/// cancellation-policy | Cancellation policy for new requests. | cancel
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample http client component config

// clang-format on
class HttpClientCore final : public ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of components::HttpClientCore component
    static constexpr std::string_view kName = "http-client-core";

    HttpClientCore(const ComponentConfig&, const ComponentContext&);

    ~HttpClientCore() override;

    /// @cond
    // For internal use only.
    std::shared_ptr<clients::http::ClientCore> GetHttpClientCore(utils::impl::InternalTag);
    /// @endcond

    static yaml_config::Schema GetStaticConfigSchema();

private:
    void OnConfigUpdate(const dynamic_config::Snapshot& config);

    void WriteStatistics(utils::statistics::Writer& writer);

    const bool disable_pool_stats_;
    std::shared_ptr<clients::http::ClientCore> http_client_;
    concurrent::AsyncEventSubscriberScope subscriber_scope_;
    utils::statistics::Entry statistics_holder_;
};

template <>
inline constexpr bool kHasValidate<HttpClientCore> = true;

template <>
inline constexpr auto kConfigFileMode<HttpClientCore> = ConfigFileMode::kNotRequired;

}  // namespace components

USERVER_NAMESPACE_END
