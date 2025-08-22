#include <userver/utils/async.hpp>

#include <tracing/span_impl.hpp>
#include <userver/baggage/baggage_manager.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/tracing/in_place_span.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

struct SpanWrapCall::Impl {
    explicit Impl(std::string&& name, InheritVariables inherit_variables, const SourceLocation& location);

    tracing::InPlaceSpan span;
    engine::impl::task_local::Storage storage;
};

SpanWrapCall::Impl::Impl(std::string&& name, InheritVariables inherit_variables, const SourceLocation& location)
    : span(std::move(name), tracing::InPlaceSpan::DetachedTag{}, location) {
    if (!engine::current_task::IsTaskProcessorThread()) {
        return;
    }
    if (inherit_variables == InheritVariables::kYes) {
        storage.InheritFrom(engine::impl::task_local::GetCurrentStorage());
    } else {
        baggage::kInheritedBaggage.InheritTo(storage, engine::impl::task_local::InternalTag{});
    }
}

SpanWrapCall::SpanWrapCall(std::string&& name, InheritVariables inherit_variables, const SourceLocation& location)
    : pimpl_(std::move(name), inherit_variables, location) {}

void SpanWrapCall::DoBeforeInvoke() {
    engine::impl::task_local::GetCurrentStorage().InitializeFrom(std::move(pimpl_->storage));
    pimpl_->span.Get().AttachToCoroStack();
}

SpanWrapCall::~SpanWrapCall() = default;

}  // namespace utils::impl

USERVER_NAMESPACE_END
