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
/// ## Static options @ref components::HttpClientCore :
/// @include{doc} scripts/docs/en/components_schema/core/src/clients/http/component_core.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
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
};

template <>
inline constexpr bool kHasValidate<HttpClientCore> = true;

template <>
inline constexpr auto kConfigFileMode<HttpClientCore> = ConfigFileMode::kNotRequired;

}  // namespace components

USERVER_NAMESPACE_END
