#pragma once

#include <engine/async.hpp>
#include <tracing/span.hpp>

namespace utils {

/* A wrapper that obtains a Span from args, attaches it to current coroutine,
 * and applies a function to the rest of arguments.
 */
struct SpanWrapCall {
  template <typename Function, typename... Args>
  auto operator()(tracing::Span&& span, Function&& f, Args&&... args) const {
    span.AttachToCoroStack();
    return std::forward<Function>(f)(std::forward<Args>(args)...);
  }
};

template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsync(engine::TaskProcessor& task_processor,
                                 const std::string& name, Function&& f,
                                 Args&&... args) {
  tracing::Span span(name);
  span.DetachFromCoroStack();

  return engine::impl::CriticalAsync(task_processor, SpanWrapCall(),
                                     std::move(span), std::forward<Function>(f),
                                     std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
[[nodiscard]] auto Async(engine::TaskProcessor& task_processor,
                         const std::string& name, Function&& f,
                         Args&&... args) {
  tracing::Span span(name);
  span.DetachFromCoroStack();

  return engine::impl::Async(task_processor, SpanWrapCall(), std::move(span),
                             std::forward<Function>(f),
                             std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsync(const std::string& name, Function&& f,
                                 Args&&... args) {
  return utils::CriticalAsync(engine::current_task::GetTaskProcessor(), name,
                              std::forward<Function>(f),
                              std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
[[nodiscard]] auto Async(const std::string& name, Function&& f,
                         Args&&... args) {
  return utils::Async(engine::current_task::GetTaskProcessor(), name,
                      std::forward<Function>(f), std::forward<Args>(args)...);
}

}  // namespace utils
