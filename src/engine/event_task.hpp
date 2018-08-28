#pragma once

#include <functional>

#include <engine/condition_variable.hpp>
#include <engine/mutex.hpp>
#include <engine/task/async_task.hpp>

#include "task/task_processor.hpp"

namespace engine {

class EventTask {
 public:
  using EventFunc = std::function<void()>;

  EventTask(TaskProcessor& task_processor, EventFunc&& event_func);
  ~EventTask();

  void Stop();
  void Notify();
  bool IsRunning() const { return is_running_; }

 private:
  void StopAsync();
  void StartEventTask(TaskProcessor& task_processor);

  EventFunc event_func_;
  AsyncTask<void> task_;

  Mutex event_cv_mutex_;
  ConditionVariable event_cv_;
  bool is_notified_ = false;
  bool is_running_ = false;
};

}  // namespace engine
