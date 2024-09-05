#pragma once

/// @file userver/ugrpc/client/common_component.hpp
/// @brief @copybrief ugrpc::client::CommonComponent

#include <optional>

#include <grpcpp/completion_queue.h>

#include <userver/components/component_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/ugrpc/client/queue_holder.hpp>
#include <userver/ugrpc/impl/statistics_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

// clang-format off

/// @ingroup userver_components
///
/// @brief Contains common machinery that's required for all ugrpc clients
///
/// ## Static options:
/// The component name for static config is `"grpc-client-common"`.
///
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// blocking-task-processor | the task processor for blocking channel creation | -
/// native-log-level | min log level for the native gRPC library | 'error'
///
/// @see ugrpc::client::ClientFactoryComponent

// clang-format on
class CommonComponent final : public components::ComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of ugrpc::client::CommonComponent
  static constexpr std::string_view kName = "grpc-client-common";

  CommonComponent(const components::ComponentConfig& config,
                  const components::ComponentContext& context);
  ~CommonComponent() override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  friend class ClientFactoryComponent;

  engine::TaskProcessor& blocking_task_processor_;
  std::optional<QueueHolder> queue_holder_;
  grpc::CompletionQueue& queue_;
  ugrpc::impl::StatisticsStorage client_statistics_storage_;
};

}  // namespace ugrpc::client

template <>
inline constexpr bool components::kHasValidate<ugrpc::client::CommonComponent> =
    true;

USERVER_NAMESPACE_END
