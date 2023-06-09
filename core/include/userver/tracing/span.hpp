#pragma once

/// @file userver/tracing/span.hpp
/// @brief @copybrief tracing::Span

#include <optional>
#include <string_view>

#include <userver/logging/log.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/tracing/tracer_fwd.hpp>
#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/internal_tag_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

class SpanBuilder;

/// @brief Measures the execution time of the current code block, links it with
/// the parent tracing::Spans and stores that info in the log.
///
/// See @ref md_en_userver_logging for usage examples and more descriptions.
///
/// @warning Shall be created only as a local variable. Do not use it as a
/// class member!
class Span final {
 public:
  class Impl;

  explicit Span(TracerPtr tracer, std::string name, const Span* parent,
                ReferenceType reference_type,
                logging::Level log_level = logging::Level::kInfo,
                utils::impl::SourceLocation source_location =
                    utils::impl::SourceLocation::Current());

  /* Use default tracer and implicit coro local storage for parent
   * identification */
  explicit Span(std::string name,
                ReferenceType reference_type = ReferenceType::kChild,
                logging::Level log_level = logging::Level::kInfo,
                utils::impl::SourceLocation source_location =
                    utils::impl::SourceLocation::Current());

  /// @cond
  // For internal use only
  explicit Span(Span::Impl& impl);
  /// @endcond

  Span(Span&& other) noexcept;

  ~Span();

  Span& operator=(const Span&) = delete;

  Span& operator=(Span&&) = delete;

  /// @brief Returns the Span of the current task.
  ///
  /// Should not be called in non-coroutine
  /// context. Should not be called from a task with no alive Span.
  ///
  /// Rule of thumb: it is safe to call it from a task created by
  /// utils::Async/utils::CriticalAsync/utils::PeriodicTask. If current task was
  /// created with an explicit engine::impl::*Async(), you have to create a Span
  /// beforehand.
  static Span& CurrentSpan();

  /// @brief Returns nullptr if called in non-coroutine context or from a task
  /// with no alive Span; otherwise returns the Span of the current task.
  static Span* CurrentSpanUnchecked();

  /// @return A new Span attached to current Span (if any).
  static Span MakeSpan(std::string name, std::string_view trace_id,
                       std::string_view parent_span_id);

  /// @return A new Span attached to current Span (if any), sets `link`.
  static Span MakeSpan(std::string name, std::string_view trace_id,
                       std::string_view parent_span_id, std::string link);

  /// Create a child which can be used independently from the parent.
  ///
  /// The child shares no state with its parent. If you need to run code in
  /// parallel, create a child span and use the child in a separate task.
  Span CreateChild(std::string name) const;

  Span CreateFollower(std::string name) const;

  /// @brief Creates a tracing::ScopeTime attached to the span.
  ///
  /// Note that
  ///
  /// @code
  ///   auto scope = tracing::Span::CurrentSpan().CreateScopeTime();
  /// @endcode
  ///
  /// is equivalent to
  ///
  /// @code
  ///   tracing::Span scope;
  /// @endcode
  ScopeTime CreateScopeTime();

  /// @brief Creates a tracing::ScopeTime attached to the Span and starts
  /// measuring execution time.
  ///
  /// Note that
  ///
  /// @code
  ///   auto scope = tracing::Span::CurrentSpan().CreateScopeTime(name);
  /// @endcode
  ///
  /// is equivalent to
  ///
  /// @code
  ///   tracing::Span scope{name};
  /// @endcode
  ScopeTime CreateScopeTime(std::string name);

  /// Returns total time elapsed for a certain scope of this span.
  /// If there is no record for the scope, returns 0.
  ScopeTime::Duration GetTotalDuration(const std::string& scope_name) const;

  /// Returns total time elapsed for a certain scope of this span.
  /// If there is no record for the scope, returns 0.
  ///
  /// Prefer using Span::GetTotalDuration()
  ScopeTime::DurationMillis GetTotalElapsedTime(
      const std::string& scope_name) const;

  /// Add a tag that is used on each logging in this Span and all
  /// future children.
  void AddTag(std::string key, logging::LogExtra::Value value);

  /// Add a tag that is used on each logging in this Span and all
  /// future children. It will not be possible to change its value.
  void AddTagFrozen(std::string key, logging::LogExtra::Value value);

  /// Add a tag that is local to the Span (IOW, it is not propagated to
  /// future children) and logged only once in the destructor of the Span.
  void AddNonInheritableTag(std::string key, logging::LogExtra::Value value);

  /// @brief Sets level for tags logging
  void SetLogLevel(logging::Level log_level);

  /// @brief Returns level for tags logging
  logging::Level GetLogLevel() const;

  /// @brief Sets the local log level that disables logging of this span if
  /// the local log level set and greater than the main log level of the Span.
  void SetLocalLogLevel(std::optional<logging::Level> log_level);

  /// @brief Returns the local log level that disables logging of this span if
  /// it is set and greater than the main log level of the Span.
  std::optional<logging::Level> GetLocalLogLevel() const;

  /// Set link. Can be called only once.
  void SetLink(std::string link);

  /// Set parent_link. Can be called only once.
  void SetParentLink(std::string parent_link);

  std::string GetLink() const;

  std::string GetParentLink() const;

  const std::string& GetTraceId() const;
  const std::string& GetSpanId() const;
  const std::string& GetParentId() const;

  void LogTo(logging::LogHelper& log_helper) const&;

  /// @returns true if this span would be logged with the current local and
  /// global log levels to the default logger.
  bool ShouldLogDefault() const noexcept;

  /// Detach the Span from current engine::Task so it is not
  /// returned by CurrentSpan() any more.
  void DetachFromCoroStack();

  /// Attach the Span to current engine::Task so it is returned
  /// by CurrentSpan().
  void AttachToCoroStack();

  std::chrono::system_clock::time_point GetStartSystemTime() const;

  /// @cond
  void AddTags(const logging::LogExtra&, utils::InternalTag);

  impl::TimeStorage& GetTimeStorage();
  /// @endcond

 private:
  struct OptionalDeleter {
    void operator()(Impl*) const noexcept;

    static OptionalDeleter ShouldDelete() noexcept;

    static OptionalDeleter DoNotDelete() noexcept;

   private:
    explicit OptionalDeleter(bool do_delete) : do_delete(do_delete) {}

    const bool do_delete;
  };

  friend class SpanBuilder;

  explicit Span(std::unique_ptr<Impl, OptionalDeleter>&& pimpl);

  std::string GetTag(std::string_view tag) const;

  std::unique_ptr<Impl, OptionalDeleter> pimpl_;
};

logging::LogHelper& operator<<(logging::LogHelper& lh, const Span& span);

}  // namespace tracing

USERVER_NAMESPACE_END
