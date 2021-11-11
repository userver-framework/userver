#pragma once

/// @file userver/ugrpc/server/queue_holder.hpp
/// @brief @copybrief ugrpc::server::QueueHolder

#include <grpcpp/completion_queue.h>
#include <grpcpp/server_builder.h>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @brief Manages a gRPC completion queue, usable in handlers and clients
/// @note During shutdown, `Manager`s must be destroyed before reactors
class QueueHolder final {
 public:
  explicit QueueHolder(::grpc::ServerBuilder& builder);

  QueueHolder(QueueHolder&&) = delete;
  QueueHolder& operator=(QueueHolder&&) = delete;
  ~QueueHolder();

  ::grpc::ServerCompletionQueue& GetQueue();

 private:
  struct Impl;
  utils::FastPimpl<Impl, 32, 8> impl_;
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
