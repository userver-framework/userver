#pragma once

#include <userver/engine/deadline.hpp>
#include <userver/engine/task/shared_task_with_result.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskFactory final {
 public:
  template <template <typename> typename TaskType, typename Function,
            typename... Args>
  [[nodiscard]] static auto MakeTaskWithResult(TaskProcessor& task_processor,
                                        Task::Importance importance,
                                        Deadline deadline, Function&& f,
                                        Args&&... args) {
    using WrappedCall = decltype(utils::impl::WrapCall(std::forward<Function>(f), std::forward<Args>(args)...));
    using ResType = decltype(std::declval<WrappedCall>()->Retrieve());

    auto a = utils::make_intrusive_ptr<TaskContextWithPayload<Function, Args...>>(task_processor, importance, Task::WaitMode::kSingleWaiter,
                                                               deadline, std::forward<Function>(f), std::forward<Args>(args)...);

    return TaskType<ResType>{std::move(a)};


    /*auto wrapped_call_ptr = utils::impl::WrapCall(std::forward<Function>(f),
                                                  std::forward<Args>(args)...);
    using ResultType = decltype(wrapped_call_ptr->Retrieve());
    return TaskType<ResultType>(task_processor, importance, deadline,
                                std::move(wrapped_call_ptr));*/
  }
};

}

USERVER_NAMESPACE_END
