#include <engine/task/context_timer.hpp>

#include <chrono>

#include <engine/ev/async_payload_base.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <engine/ev/data_pipe_to_ev.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

enum class Action {
  kCancel,
  kWakeupByEpoch,
};

template <class Derived>
class Finalizer : public ev::SingleShotAsyncPayload<Finalizer<Derived>> {
 public:
  void DoPerformAndRelease() {
    static_cast<Derived*>(this)->DoFinalizeInEvThread();
  }
  Finalizer& GetFinalizer() noexcept { return *this; }
};

template <class Derived>
class TimerArmer : public ev::MultiShotAsyncPayload<TimerArmer<Derived>> {
 public:
  void DoPerformAndRelease() {
    static_cast<Derived*>(this)->DoArmTimerInEvThread();
  }
  TimerArmer& GetTimerArmer() noexcept { return *this; }
};

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ContextTimer::Impl final : public TimerArmer<Impl>,
                                 public Finalizer<Impl> {
 public:
  struct Params {
    Action action{};
    SleepState::Epoch sleep_epoch{};
    Deadline deadline{};
  };

  Impl();
  ~Impl();

  bool WasStarted() const noexcept;

  void Start(boost::intrusive_ptr<TaskContext>&& context,
             ev::TimerThreadControl& thread_control, Params params);

  void Restart(Params params);

  void Finalize();

  void DoArmTimerInEvThread();
  void DoFinalizeInEvThread();

 private:
  void StopTimerInEvThread() noexcept;

  static void OnTimer(struct ev_loop*, ev_timer* w, int) noexcept;
  static void InvokeTimerFunction(const Params& params, TaskContext& context);
  void DoOnTimer();

  boost::intrusive_ptr<TaskContext> context_;
  ev::TimerThreadControl* thread_control_ = nullptr;
  Params params_;
  ev_timer timer_{};
  ev::DataPipeToEv<Params> params_pipe_to_ev_;
};

ContextTimer::Impl::Impl() {
  timer_.data = this;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_init(&timer_, OnTimer);
}

ContextTimer::Impl::~Impl() {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  UASSERT(!ev_is_active(&timer_));
}

bool ContextTimer::Impl::WasStarted() const noexcept {
  return context_ && thread_control_;
}

void ContextTimer::Impl::Start(boost::intrusive_ptr<TaskContext>&& context,
                               ev::TimerThreadControl& thread_control,
                               Params params) {
  UASSERT(!context_);
  UASSERT(!thread_control_);
  context_ = std::move(context);
  thread_control_ = &thread_control;

  Restart(std::move(params));
}

void ContextTimer::Impl::Restart(Params params) {
  UASSERT(WasStarted());
  UASSERT(params.deadline.IsReachable());
  if (params.deadline.IsReached()) {
    InvokeTimerFunction(params, *context_);
    return;
  }

  params_pipe_to_ev_.Push(std::move(params));
  if (PrepareEnqueue()) {
    thread_control_->RunPayloadInEvLoopDeferred(GetTimerArmer(),
                                                params.deadline);
  }
}

void ContextTimer::Impl::Finalize() {
  if (!WasStarted()) return;

  // We cannot use *this as payload here, because with MultiShotAsyncPayload,
  // two ev runs with the same data can happen. The first run would drop
  // 'context_', potentially destroying *this. The second run would
  // use-after-free.
  thread_control_->RunPayloadInEvLoopDeferred(GetFinalizer(), {});
}

void ContextTimer::Impl::DoArmTimerInEvThread() {
  UASSERT(!engine::current_task::IsTaskProcessorThread());

  auto params = params_pipe_to_ev_.TryPop();
  if (!params) {
    return;
  }

  params_ = std::move(*params);

  using LibEvDuration = std::chrono::duration<double>;
  const auto time_left =
      std::chrono::duration_cast<LibEvDuration>(params_.deadline.TimeLeft())
          .count();

  LOG_TRACE() << "time_left=" << time_left;
  if (time_left <= 0.0) {
    // Optimization for small deadlines or high load
    DoOnTimer();
    return;
  }

  timer_.repeat = time_left;
  UASSERT(thread_control_);
  thread_control_->Again(timer_);
}

void ContextTimer::Impl::InvokeTimerFunction(const Params& params,
                                             TaskContext& context) {
  UASSERT(params.action == Action::kCancel ||
          params.action == Action::kWakeupByEpoch);
  switch (params.action) {
    case Action::kCancel:
      context.RequestCancel(TaskCancellationReason::kDeadline);
      break;
    case Action::kWakeupByEpoch:
      context.Wakeup(TaskContext::WakeupSource::kDeadlineTimer,
                     params.sleep_epoch);
      break;
  }
}

void ContextTimer::Impl::StopTimerInEvThread() noexcept {
  UASSERT(!engine::current_task::IsTaskProcessorThread());
  thread_control_->Stop(timer_);
}

void ContextTimer::Impl::DoFinalizeInEvThread() {
  UASSERT(!engine::current_task::IsTaskProcessorThread());
  StopTimerInEvThread();

  // We should reset params pipe, because it may hold resources
  // in timer func closures.
  params_pipe_to_ev_.UnsafeReset();

  context_.reset();
  // ContextTimer may be destroyed at this point
}

void ContextTimer::Impl::OnTimer(struct ev_loop*, ev_timer* w, int) noexcept {
  UASSERT(!engine::current_task::IsTaskProcessorThread());

  auto* ev_timer = static_cast<Impl*>(w->data);
  UASSERT(ev_timer != nullptr);
  ev_timer->DoOnTimer();
}

void ContextTimer::Impl::DoOnTimer() {
  UASSERT(!engine::current_task::IsTaskProcessorThread());

  try {
    InvokeTimerFunction(params_, *context_);  // called in event loop
  } catch (const std::exception& ex) {
    LOG_ERROR() << "exception in ContextTimer::Impl::DoOnTimer(): " << ex;
  }
  StopTimerInEvThread();
}

ContextTimer::ContextTimer() = default;

ContextTimer::~ContextTimer() = default;

bool ContextTimer::WasStarted() const noexcept { return impl_->WasStarted(); }

void ContextTimer::StartCancel(boost::intrusive_ptr<TaskContext> context,
                               ev::TimerThreadControl& thread_control,
                               Deadline deadline) {
  impl_->Start(std::move(context), thread_control,
               {Action::kCancel, SleepState::Epoch{}, deadline});
}

void ContextTimer::StartWakeup(boost::intrusive_ptr<TaskContext> context,
                               ev::TimerThreadControl& thread_control,
                               Deadline deadline,
                               SleepState::Epoch sleep_epoch) {
  impl_->Start(std::move(context), thread_control,
               {Action::kWakeupByEpoch, sleep_epoch, deadline});
}

void ContextTimer::RestartCancel(Deadline deadline) {
  impl_->Restart({Action::kCancel, SleepState::Epoch{}, deadline});
}

void ContextTimer::RestartWakeup(Deadline deadline,
                                 SleepState::Epoch sleep_epoch) {
  impl_->Restart({Action::kWakeupByEpoch, sleep_epoch, deadline});
}

void ContextTimer::Finalize() noexcept {
  try {
    impl_->Finalize();
  } catch (const std::exception& e) {
    LOG_ERROR() << "Exception while stopping the timer: " << e;
  }
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
