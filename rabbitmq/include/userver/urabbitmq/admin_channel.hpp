#pragma once

/// @file userver/urabbitmq/admin_channel.hpp
/// @brief Administrative interface for the broker.

#include <memory>
#include <string>

#include <userver/engine/deadline.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/flags.hpp>

#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class ChannelPtr;

/// @brief Administrative interface for the broker.
/// Use this class to setup your exchanges/queues/bindings.
///
/// Usually retrieved from `Client`
class AdminChannel final {
 public:
  AdminChannel(ChannelPtr&& channel);
  ~AdminChannel();

  AdminChannel(AdminChannel&& other) noexcept;

  /// @brief Declare an exchange.
  ///
  /// @param exchange name of the exchange
  /// @param type exchange type
  /// @param flags exchange flags
  /// @param deadline execution deadline
  void DeclareExchange(const Exchange& exchange, Exchange::Type type,
                       utils::Flags<Exchange::Flags> flags,
                       engine::Deadline deadline);

  /// @brief overload of DeclareExchange
  void DeclareExchange(const Exchange& exchange, Exchange::Type type,
                       engine::Deadline deadline) {
    DeclareExchange(exchange, type, {}, deadline);
  }

  /// @brief overload of DeclareExchange
  void DeclareExchange(const Exchange& exchange, engine::Deadline deadline) {
    DeclareExchange(exchange, Exchange::Type::kFanOut, {}, deadline);
  }

  /// @brief Declare a queue.
  ///
  /// @param queue name of the queue
  /// @param flags queue flags
  /// @param deadline execution deadline
  void DeclareQueue(const Queue& queue, utils::Flags<Queue::Flags> flags,
                    engine::Deadline deadline);

  /// @brief overload of DeclareQueue
  void DeclareQueue(const Queue& queue, engine::Deadline deadline) {
    DeclareQueue(queue, {}, deadline);
  }

  /// @brief Bind a queue to an exchange.
  ///
  /// @param exchange the source exchange
  /// @param queue the target queue
  /// @param routing_key the routing key
  /// @param deadline execution deadline
  void BindQueue(const Exchange& exchange, const Queue& queue,
                 const std::string& routing_key, engine::Deadline deadline);

  /// @brief Remove an exchange.
  ///
  /// @param exchange name of the exchange to remove
  /// @param deadline execution deadline
  void RemoveExchange(const Exchange& exchange, engine::Deadline deadline);

  /// @brief Remove a queue.
  ///
  /// @param queue name of the queue to remove
  /// @param deadline execution deadline
  void RemoveQueue(const Queue& queue, engine::Deadline deadline);

 private:
  utils::FastPimpl<ChannelPtr, 32, 8> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END