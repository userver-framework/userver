#include "event_task.hpp"

#include <cassert>

#include <engine/async.hpp>
#include <logging/log.hpp>

namespace engine {

EventTask::EventTask(TaskProcessor& task_processor, EventFunc&& event_func)
    : event_func_(std::move(event_func)) {
  if (!event_func_) throw std::logic_error("event_func is null");
  StartEventTask(task_processor);
}

EventTask::~EventTask() { Stop(); }

void EventTask::Stop() {
  StopAsync();
  std::unique_lock<std::mutex> lock(stop_mutex_);
  event_task_finished_cv_.Wait(lock,
                               [this]() { return is_event_task_finished_; });
}

void EventTask::StopAsync() {
  {
    std::lock_guard<std::mutex> lock(event_cv_mutex_);
    if (!is_running_) return;
    is_running_ = false;
  }
  event_cv_.NotifyAll();
}

void EventTask::Notify() {
  {
    std::lock_guard<std::mutex> lock(event_cv_mutex_);
    is_notified_ = true;
  }
  event_cv_.NotifyAll();
}

void EventTask::StartEventTask(TaskProcessor& task_processor) {
  is_running_ = true;
  is_event_task_finished_ = false;
  Async(task_processor, [this]() {
    while (is_running_) {
      {
        std::unique_lock<std::mutex> lock(event_cv_mutex_);
        event_cv_.Wait(lock, [this]() { return !is_running_ || is_notified_; });
        if (!is_running_) break;
        assert(is_notified_);
        is_notified_ = false;
      }
      try {
        event_func_();
      } catch (const std::exception& ex) {
        LOG_ERROR() << "exception in event_func: " << ex.what();
      }
    }

    {
      std::lock_guard<std::mutex> lock(stop_mutex_);
      is_event_task_finished_ = true;
    }
    event_task_finished_cv_.NotifyAll();
  });
}

}  // namespace engine
