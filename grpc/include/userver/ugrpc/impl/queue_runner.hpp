#pragma once

#include <grpcpp/completion_queue.h>

#include <userver/engine/single_use_event.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

class QueueRunner final {
 public:
  explicit QueueRunner(grpc::CompletionQueue& queue);
  ~QueueRunner();

 private:
  grpc::CompletionQueue& queue_;
  engine::SingleUseEvent completion_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
