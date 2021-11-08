#pragma once

/// @file userver/tracing/span.hpp
/// @brief @copybrief tracing::Span

#include <optional>
#include <string_view>

#include <userver/logging/log.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/tracing/tracer_fwd.hpp>
#include <userver/utils/internal_tag_fwd.hpp>
#include <userver/utils/prof.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

/// @brief Measures the execution time of the current code block, links it with
/// the parent tracing::Spans and stores that info in the log.
///
/// See @ref md_en_userver_logging for usage examples and more descriptions.
///
/// @warning Shall be created only as a local variable. Do not use it as a
/// class member!
class Span final {
 public:
  using RealMilliseconds = TimeStorage::RealMilliseconds;
  class Impl;

  explicit Span(TracerPtr tracer, std::string name, const Span* parent,
                ReferenceType reference_type,
                logging::Level log_level = logging::Level::kInfo);

  /* Use default tracer and implicit coro local storage for parent
   * identification */
  explicit Span(std::string name,
                ReferenceType reference_type = ReferenceType::kChild,
                logging::Level log_level = logging::Level::kInfo);

  // For internal use only
  explicit Span(Span::Impl& impl);

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

  static Span MakeSpan(std::string name, std::string_view trace_id,
                       std::string_view parent_span_id);

  /** Create a child which can be used independently from the parent.
   * The child share no state with its parent. If you need to run code in
   * parallel, create a child span and use the child in a separate thread.
   */
  Span CreateChild(std::string name) const;

  Span CreateFollower(std::string name) const;

  ScopeTime CreateScopeTime();

  ScopeTime CreateScopeTime(std::string name);

  RealMilliseconds GetTotalElapsedTime(const std::string& scope_name) const;

  /** Add a tag that is used on each logging in this Span and all
   * future children.
   */
  void AddTag(std::string key, logging::LogExtra::Value value);

  /** Add a tag that is used on each logging in this Span and all
   * future children. It will not be possible to change its value.
   */
  void AddTagFrozen(std::string key, logging::LogExtra::Value value);

  /** Add a tag that is local to the Span (IOW, it is not propagated to
   * future children) and logged only once in the destructor of the Span.
   */
  void AddNonInheritableTag(std::string key, logging::LogExtra::Value value);

  /// Sets level for non-inheritable tags logging
  void SetLogLevel(logging::Level log_level);

  logging::Level GetLogLevel() const;

  void SetLocalLogLevel(std::optional<logging::Level> log_level);

  std::optional<logging::Level> GetLocalLogLevel() const;

  /** Set link. Can be called only once. */
  void SetLink(std::string link);

  std::string GetLink() const;

  std::string GetParentLink() const;

  const std::string& GetTraceId() const;
  const std::string& GetSpanId() const;
  const std::string& GetParentId() const;

  void LogTo(logging::LogHelper& log_helper) const&;

  /// Detach the Span from current engine::Task so it is not
  /// returned by CurrentSpan() any more.
  void DetachFromCoroStack();

  /// Attach the Span to current engine::Task so it is returned
  /// by CurrentSpan().
  void AttachToCoroStack();

  /// @cond
  void AddTags(const logging::LogExtra&, utils::InternalTag);
  /// @endcond

 private:
  std::string GetTag(std::string_view tag) const;

  struct OptionalDeleter {
    void operator()(Impl*) const noexcept;

    static OptionalDeleter ShouldDelete() noexcept;

    static OptionalDeleter DoNotDelete() noexcept;

   private:
    OptionalDeleter(bool do_delete) : do_delete(do_delete) {}

    const bool do_delete;
  };

 private:
  std::unique_ptr<Impl, OptionalDeleter> pimpl_;
};

logging::LogHelper& operator<<(logging::LogHelper& lh, const Span& span);

}  // namespace tracing

USERVER_NAMESPACE_END
