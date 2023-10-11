#pragma once

#include <vector>

#include <grpcpp/completion_queue.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

struct CompletionQueues final {
  std::vector<grpc::CompletionQueue*> queues;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
