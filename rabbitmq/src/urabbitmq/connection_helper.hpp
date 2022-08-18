#pragma once

#include <userver/engine/deadline.hpp>
#include <userver/urabbitmq/typedefs.hpp>
#include <userver/utils/flags.hpp>

#include <urabbitmq/impl/response_awaiter.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class ConnectionPtr;

// We reuse this from Client and Channel/AdminChannel.
//
// In channel case ConnectionPtr is owned by the channel itself,
// in client case we pass temporary in methods, so connection could be released
// back to pool without waiting for the response
class ConnectionHelper final {
 public:
  [[nodiscard]] static impl::ResponseAwaiter DeclareExchange(
      const ConnectionPtr& connection, const Exchange& exchange,
      Exchange::Type type, utils::Flags<Exchange::Flags> flags,
      engine::Deadline deadline);

  [[nodiscard]] static impl::ResponseAwaiter DeclareQueue(
      const ConnectionPtr& connection, const Queue& queue,
      utils::Flags<Queue::Flags> flags, engine::Deadline deadline);

  [[nodiscard]] static impl::ResponseAwaiter BindQueue(
      const ConnectionPtr& connection, const Exchange& exchange,
      const Queue& queue, const std::string& routing_key,
      engine::Deadline deadline);

  [[nodiscard]] static impl::ResponseAwaiter RemoveExchange(
      const ConnectionPtr& connection, const Exchange& exchange,
      engine::Deadline deadline);

  [[nodiscard]] static impl::ResponseAwaiter RemoveQueue(
      const ConnectionPtr& connection, const Queue& queue,
      engine::Deadline deadline);

  static void Publish(const ConnectionPtr& connection, const Exchange& exchange,
                      const std::string& routing_key,
                      const std::string& message, MessageType type,
                      engine::Deadline deadline);

  [[nodiscard]] static impl::ResponseAwaiter PublishReliable(
      const ConnectionPtr& connection, const Exchange& exchange,
      const std::string& routing_key, const std::string& message,
      MessageType type, engine::Deadline deadline);

 private:
  template <typename Func>
  static impl::ResponseAwaiter WithSpan(const char* name, Func&& fn) {
    tracing::Span span{name};

    auto awaiter = fn();
    awaiter.SetSpan(std::move(span));

    return awaiter;
  }
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
