#include "timer.hpp"

#include <mutex>

#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/make_intrusive_ptr.hpp>

namespace engine::ev {

class Timer::TimerImpl final : public IntrusiveRefcountedBase {
 public:
  TimerImpl(ThreadControl& thread_control, Func on_timer_func,
            double first_call_after, double repeat_every);

  void Start();
  void Stop();

  void Restart(Func on_timer_func, double first_call_after,
               double repeat_every);

 private:
  static void OnTimer(struct ev_loop*, ev_timer* w, int) noexcept;
  void DoOnTimer();

  ThreadControl thread_control_;
  Func on_timer_func_;
  ev_timer timer_{};
};

Timer::TimerImpl::TimerImpl(ThreadControl& thread_control, Func on_timer_func,
                            double first_call_after, double repeat_every)
    : thread_control_(thread_control),
      on_timer_func_(std::move(on_timer_func)) {
  if (first_call_after < 0.0) first_call_after = 0.0;
  timer_.data = this;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_timer_init(&timer_, OnTimer, first_call_after, repeat_every);
  LOG_TRACE() << "first_call_after_=" << first_call_after
              << " repeat_every_=" << repeat_every;
}

void Timer::TimerImpl::Start() {
  thread_control_.RunInEvLoopAsync(
      [](IntrusiveRefcountedBase& data) {
        auto& self = PolymorphicDowncast<TimerImpl&>(data);
        self.thread_control_.TimerStartUnsafe(self.timer_);
      },
      boost::intrusive_ptr{this});
}

void Timer::TimerImpl::Stop() {
  thread_control_.RunInEvLoopAsync(
      [](IntrusiveRefcountedBase& data) {
        auto& self = PolymorphicDowncast<TimerImpl&>(data);
        self.thread_control_.TimerStopUnsafe(self.timer_);
      },
      boost::intrusive_ptr{this});
}

void Timer::TimerImpl::Restart(Func on_timer_func, double first_call_after,
                               double repeat_every) {
  // TODO: We should keep lambda size in 16 bytes to avoid memory allocation:
  // https://godbolt.org/z/Msbeba
  boost::intrusive_ptr self = this;
  thread_control_.RunInEvLoopAsync([self = std::move(self),
                                    on_timer_func = std::move(on_timer_func),
                                    first_call_after, repeat_every]() mutable {
    auto& timer = self->timer_;
    self->thread_control_.TimerStopUnsafe(timer);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_timer_set(&timer, first_call_after, repeat_every);
    self->on_timer_func_ = std::move(on_timer_func);

    self->thread_control_.TimerStartUnsafe(timer);
  });
}

void Timer::TimerImpl::OnTimer(struct ev_loop*, ev_timer* w, int) noexcept {
  auto* ev_timer = static_cast<TimerImpl*>(w->data);
  UASSERT(ev_timer != nullptr);
  ev_timer->DoOnTimer();
}

void Timer::TimerImpl::DoOnTimer() {
  try {
    on_timer_func_();  // called in event loop
  } catch (const std::exception& ex) {
    LOG_ERROR() << "exception in on_timer_func: " << ex;
  }
}

Timer::Timer() noexcept = default;

Timer::~Timer() { Stop(); }

bool Timer::IsValid() const noexcept { return !!impl_; }

void Timer::Start(ThreadControl& thread_control, Func on_timer_func,
                  double first_call_after, double repeat_every) {
  Stop();
  impl_ = utils::make_intrusive_ptr<TimerImpl>(
      thread_control, std::move(on_timer_func), first_call_after, repeat_every);
  impl_->Start();
}

void Timer::Restart(Func on_timer_func, double first_call_after,
                    double repeat_every) {
  UASSERT(impl_);

  impl_->Restart(std::move(on_timer_func), first_call_after, repeat_every);
}

void Timer::Stop() noexcept {
  if (impl_) {
    try {
      impl_->Stop();
    } catch (const std::exception& e) {
      LOG_ERROR() << "Exception while stopping the timer: " << e;
    }

    // We should reset, because Timer::on_timer_func_ may hold smart pointher to
    // a type that holds this (circular dependency).
    impl_.reset();
  }
}

}  // namespace engine::ev
