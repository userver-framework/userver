#pragma once

#include <engine/ev/impl_in_ev_loop.hpp>

namespace engine {

class Task;
class TaskProcessor;
class TaskWorkerImpl;

class TaskWorker : public ev::ImplInEvLoop<TaskWorkerImpl> {
 public:
  TaskWorker(const ev::ThreadControl& thread_control, int id,
             TaskProcessor& task_processor);
  virtual ~TaskWorker();

  void RunTaskAsync(Task* task);
};

}  // namespace engine
