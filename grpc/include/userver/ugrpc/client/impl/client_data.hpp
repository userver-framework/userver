#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

#include <grpcpp/channel.h>
#include <grpcpp/completion_queue.h>

#include <userver/dynamic_config/source.hpp>
#include <userver/testsuite/grpc_control.hpp>
#include <userver/ugrpc/client/fwd.hpp>
#include <userver/ugrpc/client/impl/channel_cache.hpp>
#include <userver/ugrpc/client/middlewares/fwd.hpp>
#include <userver/ugrpc/impl/static_metadata.hpp>
#include <userver/ugrpc/impl/statistics.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {
class StatisticsStorage;
class CompletionQueuePoolBase;
}  // namespace ugrpc::impl

namespace ugrpc::client::impl {

/// Contains all non-code-generated dependencies for creating a gRPC client
struct ClientDependencies final {
  std::string client_name;
  std::string endpoint;
  Middlewares mws;
  ugrpc::impl::CompletionQueuePoolBase& completion_queues;
  ugrpc::impl::StatisticsStorage& statistics_storage;
  impl::ChannelCache::Token channel_token;
  const dynamic_config::Source config_source;
  testsuite::GrpcControl& testsuite_grpc;
  const dynamic_config::Key<ClientQos>* qos{nullptr};
};

struct GenericClientTag final {
  explicit GenericClientTag() = default;
};

/// The internal state of generated gRPC clients
class ClientData final {
 public:
  template <typename Service>
  using Stub = typename Service::Stub;

  ClientData() = delete;

  template <typename Service>
  ClientData(ClientDependencies&& dependencies,
             ugrpc::impl::StaticServiceMetadata metadata,
             const dynamic_config::Key<ClientQos>* legacy_builtin_qos,
             std::in_place_type_t<Service>)
      : dependencies_(std::move(dependencies)),
        metadata_(metadata),
        legacy_builtin_qos_(legacy_builtin_qos),
        service_statistics_(&GetServiceStatistics()),
        stubs_(MakeStubs<Service>(dependencies_.channel_token)) {}

  template <typename Service>
  ClientData(ClientDependencies&& dependencies, GenericClientTag,
             std::in_place_type_t<Service>)
      : dependencies_(std::move(dependencies)),
        stubs_(MakeStubs<Service>(dependencies_.channel_token)) {}

  ClientData(ClientData&&) noexcept = default;
  ClientData& operator=(ClientData&&) = delete;

  ClientData(const ClientData&) = delete;
  ClientData& operator=(const ClientData&) = delete;

  template <typename Service>
  Stub<Service>& NextStub() const {
    return *static_cast<Stub<Service>*>(NextStubPtr().get());
  }

  grpc::CompletionQueue& NextQueue() const;

  dynamic_config::Snapshot GetConfigSnapshot() const {
    return dependencies_.config_source.GetSnapshot();
  }

  ugrpc::impl::MethodStatistics& GetStatistics(std::size_t method_id) const;

  ugrpc::impl::MethodStatistics& GetGenericStatistics(
      std::string_view call_name) const;

  ChannelCache::Token& GetChannelToken() { return dependencies_.channel_token; }

  std::string_view GetClientName() const { return dependencies_.client_name; }

  const Middlewares& GetMiddlewares() const { return dependencies_.mws; }

  const ugrpc::impl::StaticServiceMetadata& GetMetadata() const;

  const testsuite::GrpcControl& GetTestsuiteControl() const {
    return dependencies_.testsuite_grpc;
  }

  const dynamic_config::Key<ClientQos>* GetClientQos() const;

 private:
  using StubDeleterType = void (*)(void*);
  using StubPtr = std::unique_ptr<void, StubDeleterType>;

  template <typename Service>
  static void StubDeleter(void* ptr) noexcept {
    delete static_cast<Stub<Service>*>(ptr);
  }

  template <typename Service>
  static utils::FixedArray<StubPtr> MakeStubs(
      impl::ChannelCache::Token& channel_token) {
    const std::size_t channel_count = channel_token.GetChannelCount();
    return utils::GenerateFixedArray(channel_count, [&](std::size_t index) {
      return StubPtr(
          Service::NewStub(channel_token.GetChannel(index)).release(),
          &StubDeleter<Service>);
    });
  }

  const StubPtr& NextStubPtr() const;

  ugrpc::impl::ServiceStatistics& GetServiceStatistics();

  ClientDependencies dependencies_;
  std::optional<ugrpc::impl::StaticServiceMetadata> metadata_{std::nullopt};
  const dynamic_config::Key<ClientQos>* legacy_builtin_qos_{nullptr};
  ugrpc::impl::ServiceStatistics* service_statistics_{nullptr};
  utils::FixedArray<StubPtr> stubs_;
};

template <typename Client>
ClientData& GetClientData(Client& client) {
  return client.impl_;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
