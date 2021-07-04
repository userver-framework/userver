#pragma once

#include <grpcpp/channel.h>
#include <grpcpp/impl/codegen/completion_queue.h>

#include <userver/engine/task/task.hpp>

namespace clients::grpc {

/// Class for managing gRPC connections and completion queue
/// An instance is created by the gRPC component or directly in unit tests.
class Manager final {
 public:
  using ChannelPtr = std::shared_ptr<::grpc::Channel>;

 public:
  Manager(engine::TaskProcessor&);
  ~Manager();

  /// Create a new gRPC channel
  ///
  /// Create a new gRPC channel using a task processor for blocking channel
  /// connection
  /// TODO cache channels and return from cache
  /// TODO pass channel credentials
  /// @param endpoint string host:port
  /// @return shared pointer to a gRPC channel
  ChannelPtr GetChannel(const std::string& endpoint);

  ::grpc::CompletionQueue& GetCompletionQueue();

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace clients::grpc
