#pragma once

#include <chrono>
#include <list>
#include <optional>
#include <string>
#include <string_view>

#include <boost/intrusive/list.hpp>

#include <userver/logging/level.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/logging/log_filepath.hpp>
#include <userver/logging/log_helper.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/prof.hpp>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DO_LOG_TO_NO_SPAN(logger, lvl)                                    \
  ::logging::LogHelper(logger, lvl, USERVER_FILEPATH, __LINE__, __func__, \
                       ::logging::LogHelper::Mode::kNoSpan)               \
      .AsLvalue()

namespace formats::json {
class ValueBuilder;
}

namespace tracing {

class Span::Impl
    : public boost::intrusive::list_base_hook<
          boost::intrusive::link_mode<boost::intrusive::auto_unlink>> {
 public:
  explicit Impl(std::string name,
                ReferenceType reference_type = ReferenceType::kChild,
                logging::Level log_level = logging::Level::kInfo);

  Impl(TracerPtr tracer, std::string name, const Span::Impl* parent,
       ReferenceType reference_type, logging::Level log_level);

  Impl(Impl&&) = default;

  ~Impl();

  TimeStorage& GetTimeStorage() {
    if (!time_storage_) time_storage_.emplace(name_);
    return *time_storage_;
  }

  void LogTo(logging::LogHelper& log_helper) const&;

  void LogTo(logging::LogHelper& log_helper) &&;

  const std::string& GetTraceId() const& noexcept { return trace_id_; }
  const std::string& GetSpanId() const& noexcept { return span_id_; }
  const std::string& GetParentId() const& noexcept { return parent_id_; }

  std::string GetTraceId() && noexcept { return std::move(trace_id_); }
  std::string GetSpanId() && noexcept { return std::move(span_id_); }
  std::string GetParentId() && noexcept { return std::move(parent_id_); }

  ReferenceType GetReferenceType() const noexcept { return reference_type_; }

  void DetachFromCoroStack();
  void AttachToCoroStack();

 private:
  void LogOpenTracing() const;
  static void AddOpentracingTags(formats::json::ValueBuilder& output,
                                 const logging::LogExtra& input);

  const std::string name_;
  const bool is_no_log_span_;
  logging::Level log_level_;
  std::optional<logging::Level> local_log_level_;

  std::shared_ptr<Tracer> tracer_;
  logging::LogExtra log_extra_inheritable_;

  Span* span_;

  std::optional<logging::LogExtra> log_extra_local_;
  std::optional<TimeStorage> time_storage_;

  const std::chrono::system_clock::time_point start_system_time_;
  const std::chrono::steady_clock::time_point start_steady_time_;

  std::string trace_id_;
  std::string span_id_;
  std::string parent_id_;
  const ReferenceType reference_type_;

  friend class Span;
};

}  // namespace tracing
