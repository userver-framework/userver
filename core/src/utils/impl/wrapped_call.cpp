#include <userver/utils/impl/wrapped_call.hpp>

#include <tracing/span_impl.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/task_inherited_data.hpp>

namespace utils::impl {

struct SpanWrapCall::Impl {
  Impl(InplaceConstructSpan&& tag);

  Impl(const Impl&) = delete;
  Impl(Impl&&) noexcept = default;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;
  ~Impl() = default;

  tracing::Span::Impl span_impl_;
  tracing::Span span_;
  impl::TaskInheritedDataStorage storage_;
};

SpanWrapCall::Impl::Impl(InplaceConstructSpan&& tag)
    : span_impl_(std::move(tag.name)),
      span_(span_impl_),
      storage_(GetTaskInheritedDataStorage()) {
  span_.DetachFromCoroStack();
}

SpanWrapCall::SpanWrapCall(InplaceConstructSpan&& tag)
    : pimpl_(std::move(tag)) {}

void SpanWrapCall::DoBeforeInvoke() {
  impl::GetTaskInheritedDataStorage() = std::move(pimpl_->storage_);
  pimpl_->span_.AttachToCoroStack();
}

SpanWrapCall::~SpanWrapCall() = default;

}  // namespace utils::impl
