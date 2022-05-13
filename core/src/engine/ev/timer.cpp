#include "timer.hpp"

#include <chrono>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/make_intrusive_ptr.hpp>

#include <engine/ev/data_pipe_to_ev.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

class Timer::TimerImpl final : public RefCountedAsyncPayloadBase<TimerImpl> {
 public:
  TimerImpl(ThreadControl& thread_control, Func on_timer_func,
            Deadline deadline);

  void Start();
  void Restart(Func on_timer_func, Deadline deadline);
  void Stop();

 private:
  void ArmTimerInEvThread();

  static void OnTimer(struct ev_loop*, ev_timer* w, int) noexcept;
  void DoOnTimer();

  struct Params {
    Func on_timer_func{};
    Deadline deadline{};
  };

  ThreadControl thread_control_;
  Params params_;
  ev_timer timer_{};

  using ParamsPipe = DataPipeToEv<Params>;
  ParamsPipe params_pipe_to_ev_;
};

Timer::TimerImpl::TimerImpl(ThreadControl& thread_control, Func on_timer_func,
                            Deadline deadline)
    : thread_control_(thread_control),
      params_{std::move(on_timer_func), deadline} {
  timer_.data = this;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_init(&timer_, OnTimer);
}

void Timer::TimerImpl::Start() {
  thread_control_.RunInEvLoopDeferred(
      [](AsyncPayloadPtr&& ptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto& self = static_cast<TimerImpl&>(*ptr);
        self.ArmTimerInEvThread();
      },
      SelfAsPayload(), params_.deadline);
}

void Timer::TimerImpl::Stop() {
  thread_control_.RunInEvLoopDeferred(
      [](AsyncPayloadPtr&& ptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto& self = static_cast<TimerImpl&>(*ptr);
        self.thread_control_.TimerStopUnsafe(self.timer_);
      },
      SelfAsPayload(), {});
}

void Timer::TimerImpl::Restart(Func on_timer_func, Deadline deadline) {
  params_pipe_to_ev_.Push({std::move(on_timer_func), deadline});
  thread_control_.RunInEvLoopDeferred(
      [](AsyncPayloadPtr&& ptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto& self = static_cast<TimerImpl&>(*ptr);
        auto params = self.params_pipe_to_ev_.TryPop();
        if (!params) {
          return;
        }

        auto& timer = self.timer_;
        self.thread_control_.TimerStopUnsafe(timer);

        self.params_ = std::move(*params);
        self.ArmTimerInEvThread();
      },
      SelfAsPayload(), deadline);
}

void Timer::TimerImpl::ArmTimerInEvThread() {
  constexpr double kDoNotRepeat = 0.0;
  using LibEvDuration = std::chrono::duration<double>;
  const auto time_left =
      std::chrono::duration_cast<LibEvDuration>(params_.deadline.TimeLeft())
          .count();

  LOG_TRACE() << "time_left=" << time_left;
  if (time_left <= 0.0) {
    // Optimization for for small deadlines or high load
    DoOnTimer();
    return;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_timer_set(&timer_, time_left, kDoNotRepeat);
  thread_control_.TimerStartUnsafe(timer_);
}

void Timer::TimerImpl::OnTimer(struct ev_loop*, ev_timer* w, int) noexcept {
  auto* ev_timer = static_cast<TimerImpl*>(w->data);
  UASSERT(ev_timer != nullptr);
  ev_timer->DoOnTimer();
}

void Timer::TimerImpl::DoOnTimer() {
  try {
    params_.on_timer_func();  // called in event loop
  } catch (const std::exception& ex) {
    LOG_ERROR() << "exception in on_timer_func: " << ex;
  }
}

Timer::Timer() noexcept = default;

Timer::~Timer() { Stop(); }

bool Timer::IsValid() const noexcept { return !!impl_; }

void Timer::Start(ThreadControl& thread_control, Func on_timer_func,
                  Deadline deadline) {
  Stop();
  impl_ = utils::make_intrusive_ptr<TimerImpl>(
      thread_control, std::move(on_timer_func), deadline);
  impl_->Start();
}

void Timer::Restart(Func on_timer_func, Deadline deadline) {
  UASSERT(impl_);

  impl_->Restart(std::move(on_timer_func), deadline);
}

void Timer::Stop() noexcept {
  if (impl_) {
    try {
      impl_->Stop();
    } catch (const std::exception& e) {
      LOG_ERROR() << "Exception while stopping the timer: " << e;
    }

    // We should reset, because Timer::on_timer_func_ may hold smart pointer to
    // a type that holds this (circular dependency).
    impl_.reset();
  }
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
