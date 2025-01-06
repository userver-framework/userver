#include <userver/ugrpc/server/impl/completion_queue_pool.hpp>

#include <grpcpp/server_builder.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

CompletionQueuePool::CompletionQueuePool(std::size_t queue_count, grpc::ServerBuilder& server_builder)
    : CompletionQueuePoolBase(utils::GenerateFixedArray(queue_count, [&server_builder](std::size_t) {
          return static_cast<std::unique_ptr<grpc::CompletionQueue>>(server_builder.AddCompletionQueue());
      })) {}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
