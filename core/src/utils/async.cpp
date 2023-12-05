#include <userver/utils/async.hpp>

#include <tracing/span_impl.hpp>
#include <userver/baggage/baggage_manager.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

struct SpanWrapCall::Impl {
  explicit Impl(std::string&& name, InheritVariables inherit_variables);

  tracing::Span::Impl span_impl_;
  tracing::Span span_;
  engine::impl::task_local::Storage storage_;
};

SpanWrapCall::Impl::Impl(std::string&& name, InheritVariables inherit_variables)
    : span_impl_(std::move(name)), span_(span_impl_) {
  if (!engine::current_task::IsTaskProcessorThread()) {
    return;
  }
  if (inherit_variables == InheritVariables::kYes) {
    storage_.InheritFrom(engine::impl::task_local::GetCurrentStorage());
  } else {
    baggage::kInheritedBaggage.InheritTo(
        storage_, engine::impl::task_local::InternalTag{});
  }
}

SpanWrapCall::SpanWrapCall(std::string&& name,
                           InheritVariables inherit_variables)
    : pimpl_(std::move(name), inherit_variables) {}

void SpanWrapCall::DoBeforeInvoke() {
  engine::impl::task_local::GetCurrentStorage().InitializeFrom(
      std::move(pimpl_->storage_));
  pimpl_->span_.AttachToCoroStack();
}

SpanWrapCall::~SpanWrapCall() = default;

}  // namespace utils::impl

USERVER_NAMESPACE_END
