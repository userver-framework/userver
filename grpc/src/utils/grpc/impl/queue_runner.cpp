#include <userver/utils/grpc/impl/queue_runner.hpp>

#include <thread>

#include <userver/utils/assert.hpp>
#include <userver/utils/thread_name.hpp>

#include <userver/utils/grpc/impl/async_method_invocation.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::grpc::impl {

namespace {

void ProcessQueue(::grpc::CompletionQueue& queue,
                  engine::SingleUseEvent& completion) noexcept {
  utils::SetCurrentThreadName("grpc-queue");

  void* tag = nullptr;
  bool ok = false;

  while (queue.Next(&tag, &ok)) {
    auto* call = static_cast<AsyncMethodInvocation*>(tag);
    UASSERT(call != nullptr);
    call->Notify(ok);
  }

  completion.Send();
}

}  // namespace

QueueRunner::QueueRunner(::grpc::CompletionQueue& queue) : queue_(queue) {
  std::thread([this] { ProcessQueue(queue_, completion_); }).detach();
}

QueueRunner::~QueueRunner() {
  queue_.Shutdown();
  completion_.WaitNonCancellable();
}

}  // namespace utils::grpc::impl

USERVER_NAMESPACE_END
