#pragma once

#include <grpcpp/completion_queue.h>

#include <userver/utils/fast_pimpl.hpp>

/// @file userver/clients/grpc/queue_holder.hpp
/// @copybrief clients::grpc::QueueHolder

USERVER_NAMESPACE_BEGIN

namespace clients::grpc {

/// @brief Manages a gRPC completion queue, usable only in clients
class QueueHolder final {
 public:
  QueueHolder();

  QueueHolder(QueueHolder&&) = delete;
  QueueHolder& operator=(QueueHolder&&) = delete;
  ~QueueHolder();

  ::grpc::CompletionQueue& GetQueue();

 private:
  struct Impl;
  utils::FastPimpl<Impl, 136, 8> impl_;
};

}  // namespace clients::grpc

USERVER_NAMESPACE_END
