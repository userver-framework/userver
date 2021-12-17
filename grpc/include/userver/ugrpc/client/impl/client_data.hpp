#pragma once

#include <memory>
#include <optional>
#include <type_traits>

#include <grpcpp/completion_queue.h>

#include <userver/ugrpc/client/channels.hpp>
#include <userver/ugrpc/client/impl/channel_cache.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

/// A helper class for generated gRPC clients
template <typename Service>
class ClientData final {
 public:
  using StubType = typename Service::Stub;

  ClientData(const std::shared_ptr<::grpc::Channel>& channel,
             ::grpc::CompletionQueue& queue)
      : stub_(StubType(channel)), queue_(queue) {}

  ClientData(impl::ChannelCache::Token channel_token,
             ::grpc::CompletionQueue& queue)
      : channel_token_(std::move(channel_token)),
        stub_(StubType(channel_token_.GetChannel())),
        queue_(queue) {}

  ClientData(ClientData&&) noexcept = default;
  ClientData& operator=(ClientData&&) = delete;

  ClientData(const ClientData&) = delete;
  ClientData& operator=(const ClientData&) = delete;

  StubType& GetStub() { return stub_; }
  ::grpc::CompletionQueue& GetQueue() { return queue_; }

 private:
  static_assert(std::is_move_constructible_v<StubType>);

  impl::ChannelCache::Token channel_token_;
  StubType stub_;
  ::grpc::CompletionQueue& queue_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
