#include <userver/ugrpc/impl/queue_runner.hpp>

#include <thread>

#include <userver/utils/assert.hpp>
#include <userver/utils/thread_name.hpp>

#include <userver/ugrpc/impl/async_method_invocation.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

namespace {

void ProcessQueue(grpc::CompletionQueue& queue,
                  engine::SingleUseEvent& completion) noexcept {
  utils::SetCurrentThreadName("grpc-queue");

  void* tag = nullptr;
  bool ok = false;

  while (queue.Next(&tag, &ok)) {
    auto* call = static_cast<EventBase*>(tag);
    UASSERT(call != nullptr);
    call->Notify(ok);
  }

  completion.Send();
}

}  // namespace

QueueRunner::QueueRunner(grpc::CompletionQueue& queue) : queue_(queue) {
  std::thread([this] { ProcessQueue(queue_, completion_); }).detach();
}

QueueRunner::~QueueRunner() {
  queue_.Shutdown();
  completion_.WaitNonCancellable();
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
