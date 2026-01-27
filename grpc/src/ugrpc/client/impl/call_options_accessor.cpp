#include <ugrpc/client/impl/call_options_accessor.hpp>

#include <userver/ugrpc/time_utils.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

void SetDeadline(
    grpc::ClientContext& client_context,
    const std::chrono::milliseconds& timeout,
    const engine::Deadline& deadline
) {
    const auto effective_deadline =
        timeout < std::chrono::milliseconds::max()
            ? std::min(engine::Deadline::FromDuration(timeout), deadline)
            : deadline;
    if (effective_deadline.IsReachable()) {
        client_context.set_deadline(effective_deadline);
    }
}

}  // namespace

std::unique_ptr<grpc::ClientContext> CallOptionsAccessor::CreateClientContext(const CallOptions& call_options) {
    auto client_context =
        call_options.client_context_factory_
            ? call_options.client_context_factory_()
            : std::make_unique<grpc::ClientContext>();
    UINVARIANT(client_context, "ClientContext should be non nullptr");

    SetDeadline(*client_context, call_options.timeout_, call_options.deadline_);

    for (const auto& [meta_key, meta_value] : call_options.metadata_) {
        client_context->AddMetadata(meta_key, meta_value);
    }

    return client_context;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
