#include "task_worker.hpp"

#include "task_worker_impl.hpp"

namespace engine {

TaskWorker::TaskWorker(const ev::ThreadControl& thread_control, int id,
                       TaskProcessor& task_processor)
    : ev::ImplInEvLoop<TaskWorkerImpl>(thread_control, id, task_processor) {}

TaskWorker::~TaskWorker() = default;

void TaskWorker::RunTaskAsync(Task* task) { impl_->RunTaskAsync(task); }

}  // namespace engine
