#include <engine/ev/thread_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

struct ev_loop* ThreadControl::GetEvLoop() const noexcept {
  return thread_.GetEvLoop();
}

void ThreadControl::RunInEvLoopAsync(OnAsyncPayload* func,
                                     AsyncPayloadPtr&& data) {
  thread_.RunInEvLoopAsync(func, std::move(data));
}

void ThreadControl::RunInEvLoopDeferred(OnAsyncPayload* func,
                                        AsyncPayloadPtr&& data,
                                        Deadline deadline) {
  thread_.RunInEvLoopDeferred(func, std::move(data), deadline);
}

bool ThreadControl::IsInEvThread() const noexcept {
  return thread_.IsInEvThread();
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
