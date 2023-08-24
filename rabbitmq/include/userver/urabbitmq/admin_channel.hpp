#pragma once

/// @file userver/urabbitmq/admin_channel.hpp
/// @brief Administrative interface for the broker.

#include <memory>

#include <userver/utils/fast_pimpl.hpp>

#include <userver/urabbitmq/broker_interface.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class ConnectionPtr;

/// @brief Administrative interface for the broker.
/// You may use this class to setup your exchanges/queues/bindings.
/// You may as well use `Client` itself instead.
///
/// You are not expected to store this class for a long time, because it takes
/// a connection from the underlying connections pool.
///
/// Usually retrieved from `Client`
class AdminChannel final : IAdminInterface {
 public:
  AdminChannel(ConnectionPtr&& channel);
  ~AdminChannel();

  AdminChannel(AdminChannel&& other) noexcept;

  void DeclareExchange(const Exchange& exchange, Exchange::Type type,
                       utils::Flags<Exchange::Flags> flags,
                       engine::Deadline deadline) override;

  void DeclareExchange(const Exchange& exchange, Exchange::Type type,
                       engine::Deadline deadline) override {
    DeclareExchange(exchange, type, {}, deadline);
  }

  void DeclareExchange(const Exchange& exchange,
                       engine::Deadline deadline) override {
    DeclareExchange(exchange, Exchange::Type::kFanOut, {}, deadline);
  }

  void DeclareQueue(const Queue& queue, utils::Flags<Queue::Flags> flags,
                    engine::Deadline deadline) override;

  void DeclareQueue(const Queue& queue, engine::Deadline deadline) override {
    DeclareQueue(queue, {}, deadline);
  }

  void BindQueue(const Exchange& exchange, const Queue& queue,
                 const std::string& routing_key,
                 engine::Deadline deadline) override;

  void RemoveExchange(const Exchange& exchange,
                      engine::Deadline deadline) override;

  void RemoveQueue(const Queue& queue, engine::Deadline deadline) override;

 private:
  utils::FastPimpl<ConnectionPtr, 32, 8> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
