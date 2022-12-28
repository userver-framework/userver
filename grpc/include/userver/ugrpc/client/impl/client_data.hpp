#pragma once

#include <cstddef>
#include <memory>
#include <utility>

#include <grpcpp/channel.h>
#include <grpcpp/completion_queue.h>

#include <userver/ugrpc/client/impl/channel_cache.hpp>
#include <userver/ugrpc/impl/statistics.hpp>
#include <userver/utils/fixed_array.hpp>
#include <userver/utils/rand.hpp>

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
        queue_(&queue),
        statistics_(&statistics) {
    const std::size_t channel_count = channel_token_.GetChannelCount();
    stubs_ = utils::GenerateFixedArray(channel_count, [&](std::size_t index) {
      return StubPtr(
          Service::NewStub(channel_token_.GetChannel(index)).release(),
          &StubDeleter<Service>);
    });
  }

  ClientData(ClientData&&) noexcept = default;
  ClientData& operator=(ClientData&&) noexcept = default;

  ClientData(const ClientData&) = delete;
  ClientData& operator=(const ClientData&) = delete;

  template <typename Service>
  Stub<Service>& NextStub() const {
    return *static_cast<Stub<Service>*>(
        stubs_[utils::RandRange(stubs_.size())].get());
  }

  grpc::CompletionQueue& GetQueue() const { return *queue_; }

  ugrpc::impl::MethodStatistics& GetStatistics(std::size_t method_id) const {
    return statistics_->GetMethodStatistics(method_id);
  }

  ChannelCache::Token& GetChannelToken() { return channel_token_; }

 private:
  using StubDeleterType = void (*)(void*);
  using StubPtr = std::unique_ptr<void, StubDeleterType>;

  template <typename Service>
  static void StubDeleter(void* ptr) noexcept {
    delete static_cast<Stub<Service>*>(ptr);
  }

  impl::ChannelCache::Token channel_token_;
  utils::FixedArray<StubPtr> stubs_;
  grpc::CompletionQueue* queue_;
  ugrpc::impl::ServiceStatistics* statistics_;
};

template <typename Client>
ClientData& GetClientData(Client& client) {
  return client.impl_;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
