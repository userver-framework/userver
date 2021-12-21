#pragma once

#include <memory>

#include <grpcpp/completion_queue.h>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

/// @brief Manages a gRPC completion queue, usable in services and clients.
/// @note During shutdown, `QueueHolder`s must be destroyed after
/// `grpc::Server::Shutdown` is called, but before ugrpc::server::Reactor
/// instances are destroyed.
class QueueHolder final {
 public:
  explicit QueueHolder(std::unique_ptr<grpc::ServerCompletionQueue>&& queue);

  QueueHolder(QueueHolder&&) = delete;
  QueueHolder& operator=(QueueHolder&&) = delete;
  ~QueueHolder();

  grpc::ServerCompletionQueue& GetQueue();

 private:
  struct Impl;
  utils::FastPimpl<Impl, 32, 8> impl_;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
