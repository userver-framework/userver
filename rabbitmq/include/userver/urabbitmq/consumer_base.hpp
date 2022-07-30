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

/// @brief Base class for your consumers.
///
/// You should derive from it and override `Process` method, which gets called
/// when a new message arrives from the broker.
///
/// @note Since `Process` is pure virtual in the base class you must
/// call `Stop` in the derived class
/// (either in destructor or whenever pleases you), otherwise you might
/// end up in a situation when pure virtual method is called, which BOOMs.
///
/// @note Library guarantees `at least once` delivery, hence some deduplication
/// might be needed ou your side.
class ConsumerBase {
 public:
  ConsumerBase(std::shared_ptr<Client> client,
               const ConsumerSettings& settings);
  virtual ~ConsumerBase();

  /// @brief Start consuming messages from the broker.
  void Start();

  /// @brief Stop consuming messages from the broker.
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
  ///
  /// @note In case message was sent to a consumer but never
  /// acknowledged back (be that ack/nack/requeue) it would stay `unacked`
  /// for `consumer_timeout` in `rabbitmq.conf`, which defaults
  /// to 30 minutes. I strongly recommend to reconsider this value
  virtual void Process(std::string message) = 0;

 private:
  std::shared_ptr<Client> client_;
  const ConsumerSettings settings_;

  std::unique_ptr<ConsumerBaseImpl> impl_;
  utils::PeriodicTask monitor_{};
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
