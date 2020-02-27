#pragma once

#include <logging/log.hpp>
#include <logging/log_extra.hpp>
#include <tracing/tracer_fwd.hpp>
#include <utils/internal_tag_fwd.hpp>
#include <utils/prof.hpp>

namespace tracing {

class Span final {
 public:
  explicit Span(TracerPtr tracer, const std::string& name, const Span* parent,
                ReferenceType reference_type,
                logging::Level log_level = logging::Level::kInfo);

  /* Use default tracer and implicit coro local storage for parent
   * identification */
  explicit Span(const std::string& name,
                ReferenceType reference_type = ReferenceType::kChild,
                logging::Level log_level = logging::Level::kInfo);

  // TODO: remove in C++17 (for guaranteed copy elision)
  Span(Span&& other) noexcept;

  ~Span();

  Span& operator=(const Span&) = delete;

  Span& operator=(Span&&) = delete;

  /// Return the Span of the current task. May not be called in non-coroutine
  /// context. May not be called from a task with no alive Span.
  /// Rule of thumb: it is safe to call it from a task created by
  /// utils::Async/utils::CriticalAsync/utils::PeriodicTask. If current task was
  /// created with an explicit engine::impl::*Async(), you have to create a Span
  /// beforehand.
  static Span& CurrentSpan();

  static Span* CurrentSpanUnchecked();

  static Span MakeSpan(const std::string& name, const std::string& trace_id,
                       const std::string& parent_span_id);

  /** Create a child which can be used independently from the parent.
   * The child share no state with its parent. If you need to run code in
   * parallel, create a child span and use the child in a separate thread.
   */
  Span CreateChild(const std::string& name) const;

  Span CreateFollower(const std::string& name) const;

  ScopeTime CreateScopeTime();

  ScopeTime CreateScopeTime(const std::string& name);

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

  /// Sets level for non-inheritable tags logging
  void SetLogLevel(logging::Level log_level);

  logging::Level GetLogLevel() const;

  void SetLocalLogLevel(boost::optional<logging::Level> log_level);

  boost::optional<logging::Level> GetLocalLogLevel() const;

  /** Set link. Can be called only once. */
  void SetLink(std::string link);

  std::string GetLink() const;

  const std::string& GetTraceId() const;
  const std::string& GetSpanId() const;
  const std::string& GetParentId() const;

  void LogTo(logging::LogHelper& log_helper) const&;

  void DetachFromCoroStack();
  void AttachToCoroStack();

  class Impl;

  /// @cond
  void AddTags(const logging::LogExtra&, utils::InternalTag);
  /// @endcond

 private:
  std::unique_ptr<Impl> pimpl_;
};

logging::LogHelper& operator<<(logging::LogHelper& lh, const Span& span);

}  // namespace tracing
