#pragma once

/// @file utils/background_task_storage.hpp
/// @brief @copybrief utils::BackgroundTasksStorage

#include <utils/async.hpp>
#include <utils/impl/wait_token_storage.hpp>

namespace utils {

/// A storage that allows one to start detached tasks and wait for their
/// finish at the storage destructor. Usable for detached tasks that capture
/// references to resources with a limited lifetime. You must guarantee that
/// the resources are available while storage is alive.
///
/// ## Usage synopsis
/// ```
/// {
///   int x;
///   // You must guarantee that 'x' is alive during 'bts' lifetime.
///   BackgroundTasksStorage bts;
///
///   bts.AsyncDetach("task", [&x]{...});
///   // bts.~BackgroundTasksStorage() waits for detached task to finish
/// }
/// ```
class BackgroundTasksStorage final {
 public:
  ~BackgroundTasksStorage() { wts_.WaitForAllTokens(); }

  BackgroundTasksStorage() = default;
  BackgroundTasksStorage(const BackgroundTasksStorage&) = delete;
  BackgroundTasksStorage& operator=(const BackgroundTasksStorage&) = delete;

  template <typename Function, typename... Args>
  void AsyncDetach(const std::string& name, Function&& f, Args&&... args) {
    utils::Async(name,
                 [f = std::move(f), token = wts_.GetToken()](auto&&... args) {
                   f(std::forward<decltype(args)>(args)...);
                 },
                 std::forward<Args>(args)...)
        .Detach();
  }

 private:
  impl::WaitTokenStorage wts_;
};

}  // namespace utils
