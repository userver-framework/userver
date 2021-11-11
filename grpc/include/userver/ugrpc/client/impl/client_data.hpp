#pragma once

#include <grpcpp/completion_queue.h>

#include <userver/ugrpc/client/channels.hpp>

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

  ClientData(ClientData&&) = delete;
  ClientData& operator=(ClientData&&) = delete;

  ClientData(const ClientData&) = delete;
  ClientData& operator=(const ClientData&) = delete;

  StubType& GetStub() { return stub_; }
  ::grpc::CompletionQueue& GetQueue() { return queue_; }

 private:
  StubType stub_;
  ::grpc::CompletionQueue& queue_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
