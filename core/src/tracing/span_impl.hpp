#pragma once

#include <list>
#include <string>

#include <boost/intrusive/list.hpp>
#include <boost/optional.hpp>

#include <tracing/tracer.hpp>

namespace tracing {

class Span;

class Span::Impl
    : public boost::intrusive::list_base_hook<
          boost::intrusive::link_mode<boost::intrusive::auto_unlink>> {
 public:
  Impl(TracerPtr tracer, const std::string& name, const Span::Impl* parent,
       ReferenceType reference_type, logging::Level log_level);

  Impl(Impl&&) = default;

  ~Impl();

  TimeStorage& GetTimeStorage() {
    if (!time_storage_) time_storage_.emplace(name_);
    return time_storage_.get();
  }

  void LogTo(logging::LogHelper& log_helper) const&;

  void LogTo(logging::LogHelper& log_helper) &&;

  const std::string& GetTraceId() const& { return trace_id_; }
  const std::string& GetSpanId() const& { return span_id_; }
  const std::string& GetParentId() const& { return parent_id_; }

  std::string&& GetTraceId() && { return std::move(trace_id_); }
  std::string&& GetSpanId() && { return std::move(parent_id_); }
  std::string&& GetParentId() && { return std::move(span_id_); }

  ReferenceType GetReferenceType() const { return reference_type_; }

  void DetachFromCoroStack();
  void AttachToCoroStack();

 private:
  logging::Level log_level_;
  boost::optional<logging::Level> local_log_level_;

  std::shared_ptr<Tracer> tracer;
  logging::LogExtra log_extra_inheritable;

  Span* span_;
  const std::string name_;

  boost::optional<logging::LogExtra> log_extra_local_;
  boost::optional<TimeStorage> time_storage_;

  const std::chrono::system_clock::time_point start_system_time_;
  const std::chrono::steady_clock::time_point start_steady_time_;

  std::string trace_id_;
  std::string span_id_;
  std::string parent_id_;
  const ReferenceType reference_type_;

  friend class Span;
};

}  // namespace tracing
