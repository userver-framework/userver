#pragma once

/// @file userver/urabbitmq/consumer_base.hpp
/// @brief Base class for your consumers.

#include <memory>

#include <userver/utils/periodic_task.hpp>

#include <userver/urabbitmq/consumer_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class Client;
class ConsumerBaseImpl;

/// @ingroup userver_base_classes
///
/// @brief Base class for your consumers.
/// You should derive from it and override `Process` method, which gets called
/// when a new message arrives from the broker.
///
/// If your configuration is known upfront and doesn't change ar runtime
/// consider using `ConsumerComponentBase` instead.
///
/// Library takes care of handling start failures and runtime failures
/// (connection breakage/broker node downtime etc.) and will try it's best to
/// restart the consumer.
///
/// @note Since messages are delivered asynchronously in the background you
/// must call `Stop` before derived class is destroyed, otherwise a race is
/// possible, when `Process` is called concurrently with
/// derived class destructor, which is UB.
///
/// @note Library guarantees `at least once` delivery, hence some deduplication
/// might be needed ou your side.
class ConsumerBase {
 public:
  ConsumerBase(std::shared_ptr<Client> client,
               const ConsumerSettings& settings);
  virtual ~ConsumerBase();

  /// @brief Start consuming messages from the broker.
  /// Calling this method on running consumer has no effect.
  ///
  /// Should not throw, in case of initial setup failure library will restart
  /// the consumer in the background.
  void Start();

  /// @brief Stop consuming messages from the broker.
  /// Calling this method on stopped consumer has no effect.
  ///
  /// @note You must call this method before your derived class is destroyed,
  /// otherwise it's UB.
  void Stop();

 protected:
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
  std::shared_ptr<Client> client_;
  const ConsumerSettings settings_;

  std::unique_ptr<ConsumerBaseImpl> impl_;
  utils::PeriodicTask monitor_{};
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
