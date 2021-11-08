#include <userver/server/grpc/queue_holder.hpp>

#include <memory>
#include <utility>

#include <userver/utils/assert.hpp>
#include <userver/utils/grpc/impl/queue_runner.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::grpc {

struct QueueHolder::Impl final {
  explicit Impl(std::unique_ptr<::grpc::ServerCompletionQueue> queue)
      : queue(std::move(queue)) {
    UASSERT(this->queue);
  }

  std::unique_ptr<::grpc::ServerCompletionQueue> queue;
  utils::grpc::impl::QueueRunner queue_runner{*queue};
};

QueueHolder::QueueHolder(::grpc::ServerBuilder& builder)
    : impl_(builder.AddCompletionQueue()) {}

QueueHolder::~QueueHolder() = default;

::grpc::ServerCompletionQueue& QueueHolder::GetQueue() { return *impl_->queue; }

}  // namespace server::grpc

USERVER_NAMESPACE_END
