#include <userver/ugrpc/impl/completion_queue_pool_base.hpp>

#include <type_traits>

#include <userver/ugrpc/impl/queue_runner.hpp>
#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

static_assert(std::has_virtual_destructor_v<grpc::CompletionQueue>);

CompletionQueuePoolBase::CompletionQueuePoolBase(utils::FixedArray<std::unique_ptr<grpc::CompletionQueue>> queues)
    : queues_(std::move(queues)), queue_runners_(utils::GenerateFixedArray(queues_.size(), [&](std::size_t idx) {
          return QueueRunner{*queues_[idx]};
      })) {}

CompletionQueuePoolBase::~CompletionQueuePoolBase() = default;

grpc::CompletionQueue& CompletionQueuePoolBase::NextQueue() { return *queues_[utils::RandRange(queues_.size())]; }

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
