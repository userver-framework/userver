#pragma once

#include <memory>
#include <utility>

#include <grpcpp/channel.h>
#include <grpcpp/completion_queue.h>

#include <userver/ugrpc/client/impl/channel_cache.hpp>
#include <userver/ugrpc/impl/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

/// A helper class for generated gRPC clients
class ClientData final {
 public:
  template <typename Service>
  using Stub = typename Service::Stub;

  ClientData() = delete;

  template <typename Service>
  ClientData(impl::ChannelCache::Token channel_token,
             grpc::CompletionQueue& queue,
             ugrpc::impl::ServiceStatistics& statistics,
             std::in_place_type_t<Service>)
      : channel_token_(std::move(channel_token)),
        stub_(Service::NewStub(channel_token_.GetChannel()).release(),
              &StubDeleter<Service>),
        queue_(&queue),
        statistics_(&statistics) {}

  ClientData(ClientData&&) noexcept = default;
  ClientData& operator=(ClientData&&) noexcept = default;

  ClientData(const ClientData&) = delete;
  ClientData& operator=(const ClientData&) = delete;

  template <typename Service>
  Stub<Service>& GetStub() {
    return *static_cast<Stub<Service>*>(stub_.get());
  }

  grpc::CompletionQueue& GetQueue() { return *queue_; }

  ugrpc::impl::MethodStatistics& GetStatistics(std::size_t method_id) {
    return statistics_->GetMethodStatistics(method_id);
  }

  grpc::Channel& GetChannel() { return *channel_token_.GetChannel(); }

 private:
  using StubDeleterType = void (*)(void*);

  template <typename Service>
  static void StubDeleter(void* ptr) noexcept {
    delete static_cast<Stub<Service>*>(ptr);
  }

  impl::ChannelCache::Token channel_token_;
  std::unique_ptr<void, StubDeleterType> stub_;
  grpc::CompletionQueue* queue_;
  ugrpc::impl::ServiceStatistics* statistics_;
};

template <typename Client>
ClientData& GetClientData(Client& client) {
  return client.impl_;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
