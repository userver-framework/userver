#include <userver/ugrpc/client/queue_holder.hpp>

#include <userver/ugrpc/impl/queue_runner.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

struct QueueHolder::Impl final {
  grpc::CompletionQueue queue;
  ugrpc::impl::QueueRunner queue_runner{queue};
};

QueueHolder::QueueHolder() = default;

QueueHolder::~QueueHolder() = default;

grpc::CompletionQueue& QueueHolder::GetQueue() { return impl_->queue; }

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
