#pragma once

#include <clients/grpc/stream.hpp>

namespace clients::grpc {

// Base class for GRPC service clients
template <typename Service>
class ServiceClient {
 public:
  using StubType = typename Service::Stub;
  using StubPtr = std::unique_ptr<StubType>;
  using ChannelPtr = std::shared_ptr<::grpc::Channel>;
  using CompletionQueue = ::grpc::CompletionQueue;

  //  ServiceClient() = default;
  ServiceClient(ChannelPtr channel, CompletionQueue& queue)
      : stub_{Service::NewStub(channel)}, queue_{queue} {}

  ServiceClient(const ServiceClient&) = delete;
  ServiceClient(ServiceClient&&) = default;

  ServiceClient& operator=(const ServiceClient&) = delete;
  ServiceClient& operator=(ServiceClient&&) = default;

 protected:
  template <typename PrepareFunc, typename... Args>
  auto Prepare(PrepareFunc prepare_func, Args&&... args) {
    return detail::PrepareInvocation(stub_.get(), prepare_func, queue_,
                                     std::forward<Args>(args)...);
  }

 private:
  StubPtr stub_;
  CompletionQueue& queue_;
};

}  // namespace clients::grpc
