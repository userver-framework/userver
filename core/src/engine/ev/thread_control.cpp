#include <engine/ev/thread_control.hpp>

#include <engine/ev/thread.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

struct ev_loop* ThreadControl::GetEvLoop() const noexcept {
  return thread_.GetEvLoop();
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Start(ev_async& w) noexcept {
  UASSERT(IsInEvThread());
  ev_async_start(GetEvLoop(), &w);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Stop(ev_async& w) noexcept {
  UASSERT(IsInEvThread());
  ev_async_stop(GetEvLoop(), &w);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Send(ev_async& w) noexcept {
  ev_async_send(GetEvLoop(), &w);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Start(ev_timer& w) noexcept {
  UASSERT(IsInEvThread());
  ev_now_update(GetEvLoop());
  ev_timer_start(GetEvLoop(), &w);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Stop(ev_timer& w) noexcept {
  UASSERT(IsInEvThread());
  ev_timer_stop(GetEvLoop(), &w);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Again(ev_timer& w) noexcept {
  UASSERT(IsInEvThread());
  ev_now_update(GetEvLoop());
  ev_timer_again(GetEvLoop(), &w);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Start(ev_io& w) noexcept {
  UASSERT(IsInEvThread());
  ev_io_start(GetEvLoop(), &w);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Stop(ev_io& w) noexcept {
  UASSERT(IsInEvThread());
  ev_io_stop(GetEvLoop(), &w);
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
