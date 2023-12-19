#include <userver/ugrpc/server/impl/queue_holder.hpp>

#include <utility>

#include <grpcpp/server_builder.h>

#include <userver/ugrpc/impl/queue_runner.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

namespace {

struct QueueSubHolder final {
  explicit QueueSubHolder(std::unique_ptr<grpc::ServerCompletionQueue> queue)
      : queue(std::move(queue)) {}

  std::unique_ptr<grpc::ServerCompletionQueue> queue;
  ugrpc::impl::QueueRunner queue_runner{*queue};
};

}  // namespace

struct QueueHolder::Impl final {
  explicit Impl(std::size_t num, grpc::ServerBuilder& server_builder)
      : queue(utils::GenerateFixedArray(num, [&server_builder](size_t) {
          return QueueSubHolder(server_builder.AddCompletionQueue());
        })) {
    for (auto& subholder : queue)
      queues.queues.push_back(subholder.queue.get());
  }

  utils::FixedArray<QueueSubHolder> queue;
  ugrpc::impl::CompletionQueues queues;
};

QueueHolder::QueueHolder(std::size_t num, grpc::ServerBuilder& server_builder)
    : impl_(num, server_builder) {}

QueueHolder::~QueueHolder() = default;

std::size_t QueueHolder::GetSize() const { return impl_->queue.size(); }

grpc::ServerCompletionQueue& QueueHolder::GetQueue(std::size_t i) {
  return *impl_->queue[i].queue;
}

const ugrpc::impl::CompletionQueues& QueueHolder::GetQueues() {
  return impl_->queues;
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
