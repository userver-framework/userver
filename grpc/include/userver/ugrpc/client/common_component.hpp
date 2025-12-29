#pragma once

/// @file userver/ugrpc/client/common_component.hpp
/// @brief @copybrief ugrpc::client::CommonComponent

#include <optional>

#include <userver/components/component_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/ugrpc/client/impl/completion_queue_pool.hpp>
#include <userver/ugrpc/client/proxy_settings.hpp>
#include <userver/ugrpc/impl/completion_queue_pool_base.hpp>
#include <userver/ugrpc/impl/statistics_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @ingroup userver_components
///
/// @brief Contains common machinery that's required for all ugrpc clients
///
/// ## Static options of ugrpc::client::CommonComponent :
/// @include{doc} scripts/docs/en/components_schema/grpc/src/ugrpc/client/common_component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// The component name for static config is `"grpc-client-common"`.
///
/// @see ugrpc::client::ClientFactoryComponent
class CommonComponent final : public components::ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref ugrpc::client::CommonComponent
    static constexpr std::string_view kName = "grpc-client-common";

    CommonComponent(const components::ComponentConfig& config, const components::ComponentContext& context);
    ~CommonComponent() override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    friend class ClientFactoryComponent;

    engine::TaskProcessor& blocking_task_processor_;
    std::optional<impl::CompletionQueuePool> client_completion_queues_;
    ugrpc::impl::CompletionQueuePoolBase& completion_queues_;
    ugrpc::impl::StatisticsStorage client_statistics_storage_;
    ProxySettings proxy_settings_;
};

}  // namespace ugrpc::client

template <>
inline constexpr bool components::kHasValidate<ugrpc::client::CommonComponent> = true;

USERVER_NAMESPACE_END
