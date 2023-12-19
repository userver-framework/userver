#pragma once

#include <memory>

#include <grpcpp/server_builder.h>

#include <userver/ugrpc/impl/completion_queues.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

/// @brief Manages a gRPC completion queue, usable in services and clients.
/// @note During shutdown, `QueueHolder`s must be destroyed after
/// `grpc::Server::Shutdown` is called, but before ugrpc::server::Reactor
/// instances are destroyed.
class QueueHolder final {
 public:
  explicit QueueHolder(std::size_t num, grpc::ServerBuilder& server_builder);

  QueueHolder(QueueHolder&&) = delete;
  QueueHolder& operator=(QueueHolder&&) = delete;
  ~QueueHolder();

  std::size_t GetSize() const;

  grpc::ServerCompletionQueue& GetQueue(std::size_t i);

  const ugrpc::impl::CompletionQueues& GetQueues();

 private:
  struct Impl;
  utils::FastPimpl<Impl, 48, 8> impl_;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
