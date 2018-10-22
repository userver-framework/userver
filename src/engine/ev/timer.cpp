#include "timer.hpp"

#include <cassert>
#include <mutex>

#include <logging/log.hpp>

namespace engine {
namespace ev {

class Timer::TimerImpl : public std::enable_shared_from_this<Timer::TimerImpl> {
 public:
  TimerImpl(ThreadControl& thread_control, Func on_timer_func,
            double first_call_after, double repeat_every);

  void Start();
  void Stop();

 private:
  static void OnTimer(struct ev_loop*, ev_timer* w, int);
  void DoOnTimer();
  void Init();

  ThreadControl thread_control_;
  Func on_timer_func_;

  double first_call_after_;
  double repeat_every_;
  ev_timer timer_;
};

Timer::TimerImpl::TimerImpl(ThreadControl& thread_control, Func on_timer_func,
                            double first_call_after, double repeat_every)
    : thread_control_(thread_control),
      on_timer_func_(std::move(on_timer_func)),
      first_call_after_(first_call_after),
      repeat_every_(repeat_every) {
  Init();
}

void Timer::TimerImpl::Start() {
  thread_control_.RunInEvLoopAsync([ self = shared_from_this(), this ]() {
    thread_control_.TimerStartUnsafe(timer_);
  });
}

void Timer::TimerImpl::OnTimer(struct ev_loop*, ev_timer* w, int) {
  TimerImpl* ev_timer = static_cast<TimerImpl*>(w->data);
  assert(ev_timer != nullptr);
  ev_timer->DoOnTimer();
}

void Timer::TimerImpl::DoOnTimer() {
  try {
    on_timer_func_();  // called in event loop
  } catch (const std::exception& ex) {
    LOG_ERROR() << "exception in on_timer_func: " << ex.what();
  }
}

void Timer::TimerImpl::Init() {
  if (first_call_after_ < 0.0) first_call_after_ = 0.0;
  timer_.data = this;
  ev_timer_init(&timer_, OnTimer, first_call_after_, repeat_every_);
  LOG_TRACE() << "first_call_after_=" << first_call_after_
              << " repeat_every_=" << repeat_every_;
}

void Timer::TimerImpl::Stop() {
  thread_control_.RunInEvLoopAsync([ self = shared_from_this(), this ]() {
    thread_control_.TimerStopUnsafe(timer_);
  });
}

Timer::Timer() = default;

Timer::Timer(ThreadControl& thread_control, Func on_timer_func,
             double first_call_after, double repeat_every, StartMode start_mode)
    : impl_(std::make_shared<TimerImpl>(thread_control,
                                        std::move(on_timer_func),
                                        first_call_after, repeat_every)) {
  if (start_mode == StartMode::kStartNow) Start();
}

Timer::~Timer() {
  if (impl_) Stop();
}

bool Timer::IsValid() const { return !!impl_; }

void Timer::Start() { impl_->Start(); }

void Timer::Stop() { impl_->Stop(); }

}  // namespace ev
}  // namespace engine
