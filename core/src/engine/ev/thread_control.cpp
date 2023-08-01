#include <engine/ev/thread_control.hpp>

#include <engine/ev/thread.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

ThreadControlBase::ThreadControlBase(Thread& thread) noexcept
    : thread_{thread} {}

struct ev_loop* ThreadControlBase::GetEvLoop() const noexcept {
  return thread_.GetEvLoop();
}

void ThreadControlBase::RunPayloadInEvLoopAsync(
    AsyncPayloadBase& payload) noexcept {
  thread_.RunInEvLoopAsync(payload);
}

void ThreadControlBase::RunPayloadInEvLoopDeferred(AsyncPayloadBase& payload,
                                                   Deadline deadline) noexcept {
  thread_.RunInEvLoopDeferred(payload, deadline);
}

bool ThreadControlBase::IsInEvThread() const noexcept {
  return thread_.IsInEvThread();
}

std::uint8_t ThreadControlBase::GetCurrentLoadPercent() const {
  return thread_.GetCurrentLoadPercent();
}

const std::string& ThreadControlBase::GetName() const {
  return thread_.GetName();
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControlBase::DoStart(ev_timer& w) noexcept {
  UASSERT(IsInEvThread());
  ev_now_update(GetEvLoop());
  ev_timer_start(GetEvLoop(), &w);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControlBase::DoStop(ev_timer& w) noexcept {
  UASSERT(IsInEvThread());
  ev_timer_stop(GetEvLoop(), &w);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControlBase::DoAgain(ev_timer& w) noexcept {
  UASSERT(IsInEvThread());
  ev_now_update(GetEvLoop());
  ev_timer_again(GetEvLoop(), &w);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControlBase::DoStart(ev_async& w) noexcept {
  UASSERT(IsInEvThread());
  ev_async_start(GetEvLoop(), &w);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControlBase::DoStop(ev_async& w) noexcept {
  UASSERT(IsInEvThread());
  ev_async_stop(GetEvLoop(), &w);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControlBase::DoSend(ev_async& w) noexcept {
  ev_async_send(GetEvLoop(), &w);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControlBase::DoStart(ev_io& w) noexcept {
  UASSERT(IsInEvThread());
  ev_io_start(GetEvLoop(), &w);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControlBase::DoStop(ev_io& w) noexcept {
  UASSERT(IsInEvThread());
  ev_io_stop(GetEvLoop(), &w);
}

TimerThreadControl::TimerThreadControl(Thread& thread) noexcept
    : ThreadControlBase{thread} {}

// NOLINTNEXTLINE(readability-make-member-function-const)
void TimerThreadControl::Start(ev_timer& w) noexcept { DoStart(w); }

// NOLINTNEXTLINE(readability-make-member-function-const)
void TimerThreadControl::Stop(ev_timer& w) noexcept { DoStop(w); }

// NOLINTNEXTLINE(readability-make-member-function-const)
void TimerThreadControl::Again(ev_timer& w) noexcept { DoAgain(w); }

ThreadControl::ThreadControl(Thread& thread) noexcept
    : ThreadControlBase{thread} {}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Start(ev_timer& w) noexcept { DoStart(w); }

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Stop(ev_timer& w) noexcept { DoStop(w); }

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Again(ev_timer& w) noexcept { DoAgain(w); }

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Start(ev_async& w) noexcept { DoStart(w); }

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Stop(ev_async& w) noexcept { DoStop(w); }

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Send(ev_async& w) noexcept { DoSend(w); }

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Start(ev_io& w) noexcept { DoStart(w); }

// NOLINTNEXTLINE(readability-make-member-function-const)
void ThreadControl::Stop(ev_io& w) noexcept { DoStop(w); }

}  // namespace engine::ev

USERVER_NAMESPACE_END
