#include <userver/ugrpc/client/call_options.hpp>

#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

void CallOptions::SetAttempts(int attempts) { attempts_ = attempts; }

int CallOptions::GetAttempts() const { return attempts_; }

void CallOptions::SetTimeout(std::chrono::milliseconds timeout) { timeout_ = timeout; }

std::chrono::milliseconds CallOptions::GetTimeout() const { return timeout_; }

void CallOptions::AddMetadata(std::string_view meta_key, std::string_view meta_value) {
    metadata_.emplace_back(ugrpc::impl::ToGrpcString(meta_key), ugrpc::impl::ToGrpcString(meta_value));
}

void CallOptions::SetClientContextFactory(ClientContextFactory&& client_context_factory) {
    client_context_factory_ = std::move(client_context_factory);
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
