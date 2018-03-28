#include "task_worker_impl.hpp"

#include <cassert>

#include <ev.h>

#include <logging/log.hpp>

#include "task.hpp"
#include "task_processor.hpp"

namespace engine {

namespace {
class CurrentTaskHolder {
 public:
  explicit CurrentTaskHolder(Task* task) {
    CurrentTask::SetCurrentTask(task);
  }
  ~CurrentTaskHolder() { CurrentTask::SetCurrentTask(nullptr); }
};
}  // namespace

TaskWorkerImpl::TaskWorkerImpl(const ev::ThreadControl& thread_control, int id,
                               TaskProcessor& task_processor)
    : ev::ThreadControl(thread_control),
      id_(id),
      task_processor_(task_processor) {
  LOG_TRACE() << "creating worker thread " << id_ << " for task_processor "
              << task_processor.Name();
  watch_task_.data = static_cast<void*>(this);
  ev_async_init(&watch_task_, TaskWatcher);
  AsyncStart(watch_task_);
}

void TaskWorkerImpl::RunTaskAsync(Task* task) {
  assert(!task_);
  task_ = task;
  ev_async_send(GetEvLoop(), &watch_task_);
}

void TaskWorkerImpl::TaskWatcher(struct ev_loop*, ev_async* w, int) {
  auto worker = static_cast<TaskWorkerImpl*>(w->data);
  assert(worker != nullptr);
  worker->TaskWatcherImpl();
}

void TaskWorkerImpl::TaskWatcherImpl() {
  assert(task_);

  Task::State state;
  {
    CurrentTaskHolder current_task(task_);
    state = task_->RunTask();
  }

  if (state == Task::State::kWaiting) task_->Sleep();
  task_ = nullptr;

  Task* task = task_processor_.NextTask(id_);
  if (task) RunTaskAsync(task);
}

}  // namespace engine
