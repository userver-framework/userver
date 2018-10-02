#pragma once

#include <opentracing/span.h>

#include <logging/log.hpp>
#include <logging/log_extra.hpp>
#include <tracing/tracer_fwd.hpp>
#include <utils/prof.hpp>

namespace tracing {

class Span final {
 public:
  explicit Span(std::unique_ptr<opentracing::Span>, TracerPtr tracer,
                const std::string& name);

  // TODO: remove in C++17 (for guaranteed copy elision)
  Span(Span&& other) noexcept;

  ~Span();

  Span& operator=(Span&&) = delete;

  /** Create a child which can be used independently from the parent.
   * The child share no state with its parent. If you need to run code in
   * parallel, create a child span and use the child in a separate thread.
   */
  Span CreateChild(const std::string& name) const;

  ScopeTime CreateScopeTime();

  /** Add a tag that is used in this Span and all future children.
   */
  void AddTag(std::string key, logging::LogExtra::Value value);

  /** Add a tag that is used in this Span and all future children.
   * It will not be possible to change its value.
   */
  void AddTagFrozen(std::string key, logging::LogExtra::Value value);

  /** Add a tag that is local to the Span (IOW, it is not propagated to
   * future children).
   */
  void AddNonInheritableTag(std::string key, logging::LogExtra::Value value);

  /** Set link. Can be called only once. */
  void SetLink(std::string link);

  std::string GetLink() const;

  void LogTo(logging::LogHelper& log_helper) const;

  opentracing::Span& GetOpentracingSpan();

  const opentracing::Span& GetOpentracingSpan() const;

  /** A hack function that returns internals, it might be deleted in future
   * versions. Don't use it! */
  logging::LogExtra& GetInheritableLogExtra();

 private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace tracing

namespace logging {
inline LogHelper& operator<<(LogHelper& lh, const tracing::Span& span) {
  span.LogTo(lh);
  return lh;
}
}  // namespace logging

#define TRACE_TRACE(span) LOG_TRACE() << span
#define TRACE_DEBUG(span) LOG_DEBUG() << span
#define TRACE_INFO(span) LOG_INFO() << span
#define TRACE_WARNING(span) LOG_WARNING() << span
#define TRACE_ERROR(span) LOG_ERROR() << span
#define TRACE_CRITICAL(span) LOG_CRITICAL() << span
