#pragma once

#include <chrono>
#include <list>
#include <optional>
#include <string>
#include <string_view>

#include <boost/intrusive/list.hpp>

#include <userver/formats/json/string_builder.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/logging/log_filepath.hpp>
#include <userver/logging/log_helper.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/impl/source_location.hpp>

#include <tracing/time_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

inline const std::string kLinkTag = "link";
inline const std::string kParentLinkTag = "parent_link";

class Span::Impl
    : public boost::intrusive::list_base_hook<
          boost::intrusive::link_mode<boost::intrusive::auto_unlink>> {
 public:
  explicit Impl(std::string name,
                ReferenceType reference_type = ReferenceType::kChild,
                logging::Level log_level = logging::Level::kInfo,
                utils::impl::SourceLocation source_location =
                    utils::impl::SourceLocation::Current());

  Impl(TracerPtr tracer, std::string name, const Span::Impl* parent,
       ReferenceType reference_type, logging::Level log_level,
       utils::impl::SourceLocation source_location);

  Impl(Impl&&) = default;

  ~Impl();

  impl::TimeStorage& GetTimeStorage() { return time_storage_; }
  const impl::TimeStorage& GetTimeStorage() const { return time_storage_; }

  // Log this Span specifically
  void PutIntoLogger(logging::impl::TagWriter writer) &&;

  // Add the context of this Span a non-Span-specific log record
  void LogTo(logging::impl::TagWriter writer);

  const std::string& GetTraceId() const& noexcept { return trace_id_; }
  const std::string& GetSpanId() const& noexcept { return span_id_; }
  const std::string& GetParentId() const& noexcept { return parent_id_; }

  std::string GetTraceId() && noexcept { return std::move(trace_id_); }
  std::string GetSpanId() && noexcept { return std::move(span_id_); }
  std::string GetParentId() && noexcept { return std::move(parent_id_); }

  void SetTraceId(std::string&& id) noexcept { trace_id_ = std::move(id); }
  void SetSpanId(std::string&& id) noexcept { span_id_ = std::move(id); }
  void SetParentId(std::string&& id) noexcept { parent_id_ = std::move(id); }

  ReferenceType GetReferenceType() const noexcept { return reference_type_; }

  void DetachFromCoroStack();
  void AttachToCoroStack();

 private:
  void LogOpenTracing() const;
  void DoLogOpenTracing(logging::impl::TagWriter writer) const;
  static void AddOpentracingTags(formats::json::StringBuilder& output,
                                 const logging::LogExtra& input);

  static std::string GetParentIdForLogging(const Span::Impl* parent);
  bool ShouldLog() const;

  const std::string name_;
  const bool is_no_log_span_;
  logging::Level log_level_;
  std::optional<logging::Level> local_log_level_;

  std::shared_ptr<Tracer> tracer_;
  logging::LogExtra log_extra_inheritable_;

  Span* span_{nullptr};

  std::optional<logging::LogExtra> log_extra_local_;
  impl::TimeStorage time_storage_;

  const std::chrono::system_clock::time_point start_system_time_;
  const std::chrono::steady_clock::time_point start_steady_time_;

  std::string trace_id_;
  std::string span_id_;
  std::string parent_id_;
  const ReferenceType reference_type_;
  utils::impl::SourceLocation source_location_;

  friend class Span;
  friend class SpanBuilder;
  friend class TagScope;
};

// Use list instead of stack to avoid UB in case of "pop non-last item"
// in case of buggy users.
using SpanStack =
    boost::intrusive::list<Span::Impl,
                           boost::intrusive::constant_time_size<false>>;

const Span::Impl* GetParentSpanImpl();

template <typename... Args>
Span::Impl* AllocateImpl(Args&&... args) {
  return new Span::Impl(std::forward<Args>(args)...);
}

class DetachLocalSpansScope final {
 public:
  DetachLocalSpansScope() noexcept;

  DetachLocalSpansScope(DetachLocalSpansScope&&) = delete;
  DetachLocalSpansScope& operator=(DetachLocalSpansScope&&) = delete;
  ~DetachLocalSpansScope();

 private:
  SpanStack old_spans_;
};

}  // namespace tracing

USERVER_NAMESPACE_END
