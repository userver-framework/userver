#include <userver/utils/async.hpp>

#include <tracing/span_impl.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/task_inherited_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

struct SpanWrapCall::Impl {
  explicit Impl(std::string&& name);

  Impl(const Impl&) = delete;
  Impl(Impl&&) noexcept = default;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;
  ~Impl() = default;

  tracing::Span::Impl span_impl_;
  tracing::Span span_;
  impl::TaskInheritedDataStorage storage_;
};

SpanWrapCall::Impl::Impl(std::string&& name)
    : span_impl_(std::move(name)),
      span_(span_impl_),
      storage_(GetTaskInheritedDataStorage()) {
  span_.DetachFromCoroStack();
}

SpanWrapCall::SpanWrapCall(std::string&& name) : pimpl_(std::move(name)) {}

void SpanWrapCall::DoBeforeInvoke() {
  impl::GetTaskInheritedDataStorage() = std::move(pimpl_->storage_);
  pimpl_->span_.AttachToCoroStack();
}

SpanWrapCall::~SpanWrapCall() = default;

}  // namespace utils::impl

USERVER_NAMESPACE_END
