#pragma once

#include <atomic>
#include <memory>

#include <userver/components/loggable_component_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/ugrpc/server/server.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @ingroup userver_components userver_base_classes
///
/// @brief Base class for all the userver gRPC Handlers.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// task-processor | the task processor to use for responses | -
class HandlerComponentBase : public components::LoggableComponentBase {
 public:
  HandlerComponentBase(const components::ComponentConfig& config,
                       const components::ComponentContext& context);

 protected:
  /// Derived classes must store the actual handler class in a field and call
  /// Register with it
  template <typename Handler>
  void Register(std::unique_ptr<Handler>&& handler) {
    CheckRegisterIsCalledOnce();
    server_.AddHandler(std::move(handler), handler_task_processor_);
  }

 private:
  void CheckRegisterIsCalledOnce();

  Server& server_;
  engine::TaskProcessor& handler_task_processor_;
  std::atomic<bool> registered_{false};
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
