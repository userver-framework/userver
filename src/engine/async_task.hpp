#pragma once

#include <cassert>
#include <exception>
#include <memory>

#include <engine/task/task.hpp>
#include <engine/task/task_processor.hpp>
#include <logging/log.hpp>

#include "call_wrapper.hpp"
#include "future.hpp"

namespace engine {

template <typename T>
class AsyncTask : public Task {
 public:
  template <typename Function, typename... Args>
  AsyncTask(TaskProcessor& task_processor, Promise<T>&& promise, Function&& f,
            Args&&... args);

  AsyncTask(const AsyncTask&) = delete;
  AsyncTask(AsyncTask&&) = delete;
  AsyncTask& operator=(const AsyncTask&) = delete;
  AsyncTask& operator=(AsyncTask&&) = delete;

  void Run() noexcept override;
  void OnComplete() noexcept override;

 private:
  TaskProcessor::RefCounter ref_counter_;
  Promise<T> promise_;
  std::unique_ptr<impl::CallWrapperBase<T>> call_wrapper_ptr_;
};

template <>
class AsyncTask<void> : public Task {
 public:
  template <typename Function, typename... Args>
  AsyncTask(TaskProcessor& task_processor, Promise<void>&& promise,
            Function&& f, Args&&... args);

  AsyncTask(const AsyncTask&) = delete;
  AsyncTask(AsyncTask&&) = delete;
  AsyncTask& operator=(const AsyncTask&) = delete;
  AsyncTask& operator=(AsyncTask&&) = delete;

  void Run() noexcept override;
  void OnComplete() noexcept override;

 private:
  TaskProcessor::RefCounter ref_counter_;
  Promise<void> promise_;
  std::unique_ptr<impl::CallWrapperBase<void>> call_wrapper_ptr_;
};

template <typename T>
template <typename Function, typename... Args>
AsyncTask<T>::AsyncTask(TaskProcessor& task_processor, Promise<T>&& promise,
                        Function&& f, Args&&... args)
    : Task(&task_processor),
      ref_counter_(task_processor),
      promise_(std::move(promise)),
      call_wrapper_ptr_(new impl::CallWrapper<Function, Args...>(
          std::forward<Function>(f), std::forward<Args>(args)...)) {
  task_processor.AddTask(this);
}

template <typename T>
void AsyncTask<T>::Run() noexcept {
  try {
    assert(call_wrapper_ptr_ && "AsyncTask ran more than once");
    auto call_wrapper_ptr = std::move(call_wrapper_ptr_);
    promise_.SetValue(call_wrapper_ptr->Call());
  } catch (const std::exception& ex) {
    promise_.SetException(std::current_exception());
  }
}

template <typename T>
void AsyncTask<T>::OnComplete() noexcept {
  delete this;
}

template <typename Function, typename... Args>
AsyncTask<void>::AsyncTask(TaskProcessor& task_processor,
                           Promise<void>&& promise, Function&& f,
                           Args&&... args)
    : Task(&task_processor),
      ref_counter_(task_processor),
      promise_(std::move(promise)),
      call_wrapper_ptr_(new impl::CallWrapper<Function, Args...>(
          std::forward<Function>(f), std::forward<Args>(args)...)) {
  task_processor.AddTask(this);
}

void AsyncTask<void>::Run() noexcept {
  try {
    assert(call_wrapper_ptr_ && "AsyncTask ran more than once");
    auto call_wrapper_ptr = std::move(call_wrapper_ptr_);
    call_wrapper_ptr->Call();
    promise_.SetValue();
  } catch (const std::exception& ex) {
    promise_.SetException(std::current_exception());
  }
}

void AsyncTask<void>::OnComplete() noexcept { delete this; }

}  // namespace engine
