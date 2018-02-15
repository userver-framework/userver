#pragma once

#include <engine/ev/thread_control.hpp>

namespace engine {

class Task;
class TaskProcessor;

class TaskWorkerImpl : public ev::ThreadControl {
 public:
  TaskWorkerImpl(const ev::ThreadControl& thread_control, int id,
                 TaskProcessor& task_processor);

  TaskWorkerImpl(TaskWorkerImpl&) = delete;
  TaskWorkerImpl(TaskWorkerImpl&&) = delete;

  void RunTaskAsync(Task* task);
  TaskProcessor& GetTaskProcessor() { return task_processor_; }

 private:
  static void TaskWatcher(struct ev_loop*, ev_async* w, int);
  void TaskWatcherImpl();

  const int id_;
  TaskProcessor& task_processor_;

  ev_async watch_task_;

  Task* task_ = nullptr;
};

}  // namespace engine
