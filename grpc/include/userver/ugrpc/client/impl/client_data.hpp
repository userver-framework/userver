#pragma once

#include <memory>
#include <utility>

#include <grpcpp/channel.h>
#include <grpcpp/completion_queue.h>

#include <userver/ugrpc/client/impl/channel_cache.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

/// A helper class for generated gRPC clients
class ClientData final {
 public:
  template <typename Service>
  using Stub = typename Service::Stub;

  ClientData() = delete;

  template <typename Service>
  ClientData(const std::shared_ptr<grpc::Channel>& channel,
             grpc::CompletionQueue& queue, std::in_place_type_t<Service>)
      : stub_(Service::NewStub(channel).release(), &StubDeleter<Service>),
        queue_(&queue) {}

  template <typename Service>
  ClientData(impl::ChannelCache::Token channel_token,
             grpc::CompletionQueue& queue, std::in_place_type_t<Service>)
      : channel_token_(std::move(channel_token)),
        stub_(Service::NewStub(channel_token_.GetChannel()).release(),
              &StubDeleter<Service>),
        queue_(&queue) {}

  ClientData(ClientData&&) noexcept = default;
  ClientData& operator=(ClientData&&) noexcept = default;

  ClientData(const ClientData&) = delete;
  ClientData& operator=(const ClientData&) = delete;

  template <typename Service>
  Stub<Service>& GetStub() {
    return *static_cast<Stub<Service>*>(stub_.get());
  }

  grpc::CompletionQueue& GetQueue() { return *queue_; }

 private:
  using StubDeleterType = void (*)(void*);

  template <typename Service>
  static void StubDeleter(void* ptr) noexcept {
    delete static_cast<Stub<Service>*>(ptr);
  }

  impl::ChannelCache::Token channel_token_;
  std::unique_ptr<void, StubDeleterType> stub_;
  grpc::CompletionQueue* queue_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
