#include "timer_event.hpp"

#include <cassert>
#include <mutex>

#include <logging/log.hpp>

namespace engine {
namespace impl {

class TimerEvent::TimerImpl
    : public std::enable_shared_from_this<TimerEvent::TimerImpl> {
 public:
  TimerImpl(ev::ThreadControl& thread_control, Func on_timer_func,
            double first_call_after, double repeat_every);

  void Start();
  void Stop();

 private:
  static void OnTimer(struct ev_loop*, ev_timer* w, int);
  void DoOnTimer();
  void Init();

  ev::ThreadControl thread_control_;
  Func on_timer_func_;

  std::mutex mutex_;
  bool on_timer_enabled_;

  double first_call_after_;
  double repeat_every_;
  ev_timer timer_;
};

TimerEvent::TimerImpl::TimerImpl(ev::ThreadControl& thread_control,
                                 Func on_timer_func, double first_call_after,
                                 double repeat_every)
    : thread_control_(thread_control),
      on_timer_func_(std::move(on_timer_func)),
      on_timer_enabled_(false),
      first_call_after_(first_call_after),
      repeat_every_(repeat_every) {
  Init();
}

void TimerEvent::TimerImpl::Start() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (on_timer_enabled_) return;
    on_timer_enabled_ = true;
    thread_control_.RunInEvLoopAsync([ self = shared_from_this(), this ]() {
      thread_control_.TimerStartUnsafe(timer_);
    });
  }
}

void TimerEvent::TimerImpl::OnTimer(struct ev_loop*, ev_timer* w, int) {
  TimerImpl* ev_timer = static_cast<TimerImpl*>(w->data);
  assert(ev_timer != nullptr);
  ev_timer->DoOnTimer();
}

void TimerEvent::TimerImpl::DoOnTimer() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!on_timer_enabled_) return;
  }

  try {
    on_timer_func_();  // called in event loop
  } catch (const std::exception& ex) {
    LOG_ERROR() << "exception in on_timer_func: " << ex.what();
  }
}

void TimerEvent::TimerImpl::Init() {
  if (first_call_after_ < 0.0) first_call_after_ = 0.0;
  timer_.data = this;
  ev_timer_init(&timer_, OnTimer, first_call_after_, repeat_every_);
  LOG_TRACE() << "first_call_after_=" << first_call_after_
              << " repeat_every_=" << repeat_every_;
}

void TimerEvent::TimerImpl::Stop() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!on_timer_enabled_) return;
    on_timer_enabled_ = false;
    thread_control_.RunInEvLoopAsync([ self = shared_from_this(), this ]() {
      thread_control_.TimerStopUnsafe(timer_);
    });
  }
}

TimerEvent::TimerEvent(ev::ThreadControl& thread_control, Func on_timer_func,
                       double first_call_after, double repeat_every,
                       StartMode start_mode)
    : impl_(std::make_shared<TimerImpl>(thread_control,
                                        std::move(on_timer_func),
                                        first_call_after, repeat_every)) {
  if (start_mode == StartMode::kStartNow) Start();
}

TimerEvent::~TimerEvent() {
  Stop();
  impl_.reset();
}

void TimerEvent::Start() { impl_->Start(); }

void TimerEvent::Stop() { impl_->Stop(); }

}  // namespace impl
}  // namespace engine
