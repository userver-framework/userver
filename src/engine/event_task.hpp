#pragma once

#include <functional>
#include <mutex>

#include <engine/condition_variable.hpp>
#include <engine/task/task_processor.hpp>

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

  std::mutex event_cv_mutex_;
  ConditionVariable event_cv_;
  std::mutex stop_mutex_;
  ConditionVariable event_task_finished_cv_;
  bool is_event_task_finished_ = true;
  bool is_notified_ = false;
  bool is_running_ = false;
};

}  // namespace engine
