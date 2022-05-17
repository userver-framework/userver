#include "timer.hpp"

#include <chrono>

#include <engine/ev/async_payload_base.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <engine/ev/data_pipe_to_ev.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

class Timer::Impl final : public AsyncPayloadBase {
 public:
  explicit Impl(engine::impl::TaskContext& owner);
  ~Impl();

  void Start(ThreadControl thread_control, Func&& on_timer_func,
             Deadline deadline);

  void Restart(Func&& on_timer_func, Deadline deadline);

  void Stop();

  bool IsValid() const noexcept;

 private:
  static void Release(AsyncPayloadBase& base) noexcept;

  AsyncPayloadPtr SelfAsPayload() noexcept;

  void ArmTimerInEvThread();
  void StopTimerInEvThread() noexcept;

  static void OnTimer(struct ev_loop*, ev_timer* w, int) noexcept;
  void DoOnTimer();

  struct Params {
    Func on_timer_func{};
    Deadline deadline{};
  };

  engine::impl::TaskContext& owner_;
  std::optional<ThreadControl> thread_control_;
  Params params_;
  ev_timer timer_{};

  using ParamsPipe = DataPipeToEv<Params>;
  ParamsPipe params_pipe_to_ev_;
};

Timer::Impl::Impl(engine::impl::TaskContext& owner)
    : AsyncPayloadBase(&Release), owner_(owner) {
  timer_.data = this;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_init(&timer_, OnTimer);
}

Timer::Impl::~Impl() {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  UASSERT(!ev_is_active(&timer_));
}

void Timer::Impl::Start(ThreadControl thread_control, Func&& on_timer_func,
                        Deadline deadline) {
  UASSERT(!thread_control_);
  thread_control_.emplace(thread_control);
  params_ = {std::move(on_timer_func), deadline};

  thread_control_->RunInEvLoopDeferred(
      [](AsyncPayloadPtr&& data) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto& self = static_cast<Impl&>(*data);
        self.ArmTimerInEvThread();
      },
      SelfAsPayload(), params_.deadline);
}

void Timer::Impl::Restart(Func&& on_timer_func, Deadline deadline) {
  UASSERT(thread_control_);
  params_pipe_to_ev_.Push({std::move(on_timer_func), deadline});

  thread_control_->RunInEvLoopDeferred(
      [](AsyncPayloadPtr&& data) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto& self = static_cast<Impl&>(*data);
        auto params = self.params_pipe_to_ev_.TryPop();
        if (!params) {
          return;
        }

        self.params_ = std::move(*params);
        self.ArmTimerInEvThread();
      },
      SelfAsPayload(), deadline);
}

void Timer::Impl::Stop() {
  if (!thread_control_) return;

  thread_control_->RunInEvLoopDeferred(
      [](AsyncPayloadPtr&& data) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto& self = static_cast<Impl&>(*data);
        self.StopTimerInEvThread();
      },
      SelfAsPayload(), {});
}

bool Timer::Impl::IsValid() const noexcept {
  return thread_control_.has_value();
}

void Timer::Impl::Release(AsyncPayloadBase& base) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  intrusive_ptr_release(&static_cast<Impl&>(base).owner_);
}

AsyncPayloadPtr Timer::Impl::SelfAsPayload() noexcept {
  intrusive_ptr_add_ref(&owner_);
  return AsyncPayloadPtr{this};
}

void Timer::Impl::ArmTimerInEvThread() {
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

  timer_.repeat = time_left;
  thread_control_->Again(timer_);
}

void Timer::Impl::StopTimerInEvThread() noexcept {
  thread_control_->Stop(timer_);

  // We should reset 'on_timer_func', because it may hold a smart pointer to
  // TaskContext that holds 'this' (circular dependency). 'this' may be
  // destroyed here.
  params_.on_timer_func = {};
}

void Timer::Impl::OnTimer(struct ev_loop*, ev_timer* w, int) noexcept {
  auto* ev_timer = static_cast<Impl*>(w->data);
  UASSERT(ev_timer != nullptr);
  ev_timer->DoOnTimer();
}

void Timer::Impl::DoOnTimer() {
  try {
    params_.on_timer_func();  // called in event loop
  } catch (const std::exception& ex) {
    LOG_ERROR() << "exception in on_timer_func: " << ex;
  }
  StopTimerInEvThread();
}

Timer::Timer(engine::impl::TaskContext& owner) : impl_(owner) {}

Timer::~Timer() = default;

bool Timer::IsValid() const noexcept { return impl_->IsValid(); }

void Timer::Start(ThreadControl thread_control, Func&& on_timer_func,
                  Deadline deadline) {
  impl_->Start(thread_control, std::move(on_timer_func), deadline);
}

void Timer::Restart(Func&& on_timer_func, Deadline deadline) {
  impl_->Restart(std::move(on_timer_func), deadline);
}

void Timer::Stop() noexcept {
  try {
    impl_->Stop();
  } catch (const std::exception& e) {
    LOG_ERROR() << "Exception while stopping the timer: " << e;
  }
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
