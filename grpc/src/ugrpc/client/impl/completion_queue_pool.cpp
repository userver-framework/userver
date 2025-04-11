#include <userver/ugrpc/client/impl/completion_queue_pool.hpp>

#include <userver/ugrpc/impl/queue_runner.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

CompletionQueuePool::CompletionQueuePool(std::size_t queue_count)
    : CompletionQueuePoolBase(utils::GenerateFixedArray(queue_count, [](std::size_t) {
          return std::make_unique<grpc::CompletionQueue>();
      })) {}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
