#include <userver/clients/grpc/queue_holder.hpp>

#include <userver/utils/grpc/impl/queue_runner.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::grpc {

struct QueueHolder::Impl final {
  ::grpc::CompletionQueue queue;
  utils::grpc::impl::QueueRunner queue_runner{queue};
};

// explicit default-initialization to silence a clang-tidy false positive
QueueHolder::QueueHolder() : impl_() {}

QueueHolder::~QueueHolder() = default;

::grpc::CompletionQueue& QueueHolder::GetQueue() { return impl_->queue; }

}  // namespace clients::grpc

USERVER_NAMESPACE_END
