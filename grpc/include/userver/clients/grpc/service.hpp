#pragma once

#include <grpcpp/channel.h>

namespace clients::grpc {

// Base class for GRPC service clients
template <typename Service>
class ServiceClient {
 public:
  using ChannelPtr = std::shared_ptr<::grpc::Channel>;

  ServiceClient(ChannelPtr channel, ::grpc::CompletionQueue& queue)
      : stub_(Service::NewStub(channel)), queue_(&queue) {}

  ServiceClient(ServiceClient&&) = default;
  ServiceClient& operator=(ServiceClient&&) = default;

  ServiceClient(const ServiceClient&) = delete;
  ServiceClient& operator=(const ServiceClient&) = delete;

 protected:
  using StubType = typename Service::Stub;

  StubType& GetStub() { return *stub_; }
  ::grpc::CompletionQueue& GetQueue() { return *queue_; }

 private:
  std::unique_ptr<StubType> stub_;
  ::grpc::CompletionQueue* queue_;
};

}  // namespace clients::grpc
