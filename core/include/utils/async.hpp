#pragma once

#include <engine/async.hpp>
#include <tracing/span.hpp>

namespace utils {

/* A wrapper that obtains a Span from args, attaches it to current coroutine,
 * and applies a function to the rest of arguments.
 */
struct SpanWrapCall {
  template <typename Function, typename... Args>
  auto operator()(tracing::Span&& span, Function&& f, Args&&... args) {
    span.AttachToCoroStack();
    return f(std::forward<Args>(args)...);
  }
};

template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsync(const std::string& name, Function&& f,
                                 Args&&... args) {
  tracing::Span span(name);
  span.DetachFromCoroStack();

  return engine::impl::CriticalAsync(SpanWrapCall(), std::move(span),
                                     std::forward<Function>(f),
                                     std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
[[nodiscard]] auto Async(const std::string& name, Function&& f,
                         Args&&... args) {
  tracing::Span span(name);
  span.DetachFromCoroStack();

  return engine::impl::Async(SpanWrapCall(), std::move(span),
                             std::forward<Function>(f),
                             std::forward<Args>(args)...);
}

}  // namespace utils
