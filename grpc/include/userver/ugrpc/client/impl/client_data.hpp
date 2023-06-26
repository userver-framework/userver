#pragma once

#include <cstddef>
#include <memory>
#include <utility>

#include <grpcpp/channel.h>
#include <grpcpp/completion_queue.h>

#include <userver/dynamic_config/source.hpp>
#include <userver/testsuite/grpc_control.hpp>
#include <userver/ugrpc/client/impl/channel_cache.hpp>
#include <userver/ugrpc/client/middlewares/fwd.hpp>
#include <userver/ugrpc/impl/static_metadata.hpp>
#include <userver/ugrpc/impl/statistics.hpp>
#include <userver/utils/fixed_array.hpp>
#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

struct ClientParams final {
  std::string client_name;
  Middlewares mws;
  grpc::CompletionQueue& queue;
  ugrpc::impl::ServiceStatistics& statistics_storage;
  impl::ChannelCache::Token channel_token;
  const dynamic_config::Source config_source;
  testsuite::GrpcControl& testsuite_grpc;
};

/// A helper class for generated gRPC clients
class ClientData final {
 public:
  template <typename Service>
  using Stub = typename Service::Stub;

  ClientData() = delete;

  template <typename Service>
  ClientData(ClientParams&& params, ugrpc::impl::StaticServiceMetadata metadata,
             std::in_place_type_t<Service>)
      : params_(std::move(params)), metadata_(metadata) {
    const std::size_t channel_count = GetChannelToken().GetChannelCount();
    stubs_ = utils::GenerateFixedArray(channel_count, [&](std::size_t index) {
      return StubPtr(
          Service::NewStub(GetChannelToken().GetChannel(index)).release(),
          &StubDeleter<Service>);
    });
  }

  ClientData(ClientData&&) noexcept = default;
  ClientData& operator=(ClientData&&) = delete;

  ClientData(const ClientData&) = delete;
  ClientData& operator=(const ClientData&) = delete;

  template <typename Service>
  Stub<Service>& NextStub() const {
    return *static_cast<Stub<Service>*>(
        stubs_[utils::RandRange(stubs_.size())].get());
  }

  grpc::CompletionQueue& GetQueue() const { return params_.queue; }

  dynamic_config::Snapshot GetConfigSnapshot() const {
    return params_.config_source.GetSnapshot();
  }

  ugrpc::impl::MethodStatistics& GetStatistics(std::size_t method_id) const {
    return params_.statistics_storage.GetMethodStatistics(method_id);
  }

  ChannelCache::Token& GetChannelToken() { return params_.channel_token; }

  std::string_view GetClientName() const { return params_.client_name; }

  const Middlewares& GetMiddlewares() const { return params_.mws; }

  const ugrpc::impl::StaticServiceMetadata& GetMetadata() const {
    return metadata_;
  }

  const testsuite::GrpcControl& GetTestsuiteControl() const {
    return params_.testsuite_grpc;
  }

 private:
  using StubDeleterType = void (*)(void*);
  using StubPtr = std::unique_ptr<void, StubDeleterType>;

  template <typename Service>
  static void StubDeleter(void* ptr) noexcept {
    delete static_cast<Stub<Service>*>(ptr);
  }

  ClientParams params_;
  ugrpc::impl::StaticServiceMetadata metadata_;
  utils::FixedArray<StubPtr> stubs_;
};

template <typename Client>
ClientData& GetClientData(Client& client) {
  return client.impl_;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
