#include <engine/task/context_timer.hpp>

#include <chrono>

#include <engine/ev/async_payload_base.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <engine/ev/data_pipe_to_ev.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class ContextTimer::Impl final : public ev::AsyncPayloadBase {
 public:
  Impl();
  ~Impl();

  bool WasStarted() const noexcept;

  void Start(boost::intrusive_ptr<TaskContext>&& context,
             ev::ThreadControl thread_control, Func&& on_timer_func,
             Deadline deadline);

  void Restart(Func&& on_timer_func, Deadline deadline);

  void Finalize();

 private:
  static void Release(ev::AsyncPayloadBase& base) noexcept;

  ev::AsyncPayloadPtr SelfAsPayload() noexcept;

  void ArmTimerInEvThread();
  void StopTimerInEvThread() noexcept;

  static void OnTimer(struct ev_loop*, ev_timer* w, int) noexcept;
  void DoOnTimer();

  struct Params {
    Func on_timer_func{};
    Deadline deadline{};
  };

  boost::intrusive_ptr<TaskContext> context_;
  std::optional<ev::ThreadControl> thread_control_;
  Params params_;
  ev_timer timer_{};

  using ParamsPipe = ev::DataPipeToEv<Params>;
  ParamsPipe params_pipe_to_ev_;
};

ContextTimer::Impl::Impl() : ev::AsyncPayloadBase(&Release) {
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
                               ev::ThreadControl thread_control,
                               Func&& on_timer_func, Deadline deadline) {
  UASSERT(!context_);
  UASSERT(!thread_control_);
  context_ = std::move(context);
  thread_control_.emplace(thread_control);
  params_ = {std::move(on_timer_func), deadline};

  thread_control_->RunInEvLoopDeferred(
      [](ev::AsyncPayloadPtr&& data) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto& self = static_cast<Impl&>(*data);
        self.ArmTimerInEvThread();
      },
      SelfAsPayload(), params_.deadline);
}

void ContextTimer::Impl::Restart(Func&& on_timer_func, Deadline deadline) {
  UASSERT(WasStarted());
  params_pipe_to_ev_.Push({std::move(on_timer_func), deadline});

  thread_control_->RunInEvLoopDeferred(
      [](ev::AsyncPayloadPtr&& data) {
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

void ContextTimer::Impl::Finalize() {
  if (!WasStarted()) return;

  thread_control_->RunInEvLoopDeferred(
      [](ev::AsyncPayloadPtr&& data) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto& self = static_cast<Impl&>(*data);
        self.StopTimerInEvThread();

        // We should reset 'on_timer_func' and params pipe, because they may
        // hold resources in timer func closures.
        self.params_.on_timer_func = {};
        self.params_pipe_to_ev_.UnsafeReset();

        // used as a flag for context release
        self.thread_control_.reset();
      },
      SelfAsPayload(), {});
}

void ContextTimer::Impl::Release(ev::AsyncPayloadBase& base) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto& self = static_cast<Impl&>(base);
  if (!self.thread_control_) {
    self.context_.reset();
    // ContextTimer may be destroyed at this point
  }
}

ev::AsyncPayloadPtr ContextTimer::Impl::SelfAsPayload() noexcept {
  return ev::AsyncPayloadPtr{this};
}

void ContextTimer::Impl::ArmTimerInEvThread() {
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

void ContextTimer::Impl::StopTimerInEvThread() noexcept {
  thread_control_->Stop(timer_);
}

void ContextTimer::Impl::OnTimer(struct ev_loop*, ev_timer* w, int) noexcept {
  auto* ev_timer = static_cast<Impl*>(w->data);
  UASSERT(ev_timer != nullptr);
  ev_timer->DoOnTimer();
}

void ContextTimer::Impl::DoOnTimer() {
  try {
    // do not keep the function object around for much longer
    const auto on_timer_func = std::move(params_.on_timer_func);
    on_timer_func(*context_);  // called in event loop
  } catch (const std::exception& ex) {
    LOG_ERROR() << "exception in on_timer_func: " << ex;
  }
  StopTimerInEvThread();
}

ContextTimer::ContextTimer() = default;

ContextTimer::~ContextTimer() = default;

bool ContextTimer::WasStarted() const noexcept { return impl_->WasStarted(); }

void ContextTimer::Start(boost::intrusive_ptr<TaskContext> context,
                         ev::ThreadControl thread_control, Func&& on_timer_func,
                         Deadline deadline) {
  impl_->Start(std::move(context), thread_control, std::move(on_timer_func),
               deadline);
}

void ContextTimer::Restart(Func&& on_timer_func, Deadline deadline) {
  impl_->Restart(std::move(on_timer_func), deadline);
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
