#include <userver/ugrpc/server/queue_holder.hpp>

#include <memory>
#include <utility>

#include <userver/ugrpc/impl/queue_runner.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

struct QueueHolder::Impl final {
  explicit Impl(std::unique_ptr<::grpc::ServerCompletionQueue> queue)
      : queue(std::move(queue)) {
    UASSERT(this->queue);
  }

  std::unique_ptr<::grpc::ServerCompletionQueue> queue;
  ugrpc::impl::QueueRunner queue_runner{*queue};
};

QueueHolder::QueueHolder(::grpc::ServerBuilder& builder)
    : impl_(builder.AddCompletionQueue()) {}

QueueHolder::~QueueHolder() = default;

::grpc::ServerCompletionQueue& QueueHolder::GetQueue() { return *impl_->queue; }

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
