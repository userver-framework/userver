#include <userver/ugrpc/server/impl/call_state.hpp>

#include <userver/dynamic_config/source.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

tracing::Span& CallState::GetSpan() {
    UINVARIANT(span_storage, "Span is not set up for this RPC yet");
    return span_storage->Get();
}

CallState::CallState(CallParams&& params, CallKind call_kind)
    : CallParams(std::move(params)),
      statistics_scope(method_statistics),
      call_kind(call_kind),
      config_snapshot(config_source.GetSnapshot()) {}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
