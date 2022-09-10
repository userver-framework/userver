#pragma once

/// @file userver/urabbitmq/consumer_component_base.hpp
/// @brief Base component for your consumers.

#include <memory>

#include <userver/components/loggable_component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

// clang-format off
/// @ingroup userver_base_classes
///
/// @brief Base component for your consumers.
/// Basically a `ConsumerBase` but in a nice component-ish way
///
/// You should derive from it and override `Process` method, which gets called
/// when a new message arrives from the broker.
/// The consumer will be automatically started after all components are loaded
/// and stopped before all components are beginning to stop.
///
/// Library takes care of handling start failures and runtime failures
/// (connection breakage/broker node downtime etc.) and will try it's best to
/// restart the consumer.
///
/// @note Library guarantees `at least once` delivery, hence some deduplication
/// might be needed ou your side.
///
/// ## Static configuration example:
///
/// @snippet samples/rabbitmq_service/static_config.yaml  RabbitMQ consumer sample - static config
///
/// ## Static options:
/// Name             | Description
/// rabbit_name      | Name of the RabbitMQ component to use for consumption
/// queue            | Name of the queue to consume from
/// prefetch_count   | prefetch_count for the consumer, limits the amount of in-flight messages
///
// clang-format on
class ConsumerComponentBase : public components::LoggableComponentBase {
 public:
  ConsumerComponentBase(const components::ComponentConfig& config,
                        const components::ComponentContext& context);
  ~ConsumerComponentBase() override;

  static yaml_config::Schema GetStaticConfigSchema();

 protected:
  void OnAllComponentsLoaded() final;

  void OnAllComponentsAreStopping() final;

  /// @brief Override this method in derived class and implement
  /// message handling logic.
  ///
  /// If this method returns successfully message would be acked (best effort)
  /// to the broker, if this method throws the message would be requeued.
  ///
  /// Please keep in mind that it is possible for the message to be delivered
  /// again even if `Process` returns successfully: sadly we can't guarantee
  /// that `ack` ever reached the broker (network issues or unexpected shutdown,
  /// for example).
  /// It is however guaranteed for message to be requeued if `Process` fails.
  virtual void Process(std::string message) = 0;

 private:
  // This is actually just a subclass of `ConsumerBase`
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace urabbitmq

namespace components {

template <>
inline constexpr bool kHasValidate<urabbitmq::ConsumerComponentBase> = true;

}

USERVER_NAMESPACE_END
