#include <userver/ugrpc/client/impl/stub_handle.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

StubHandle::StubHandle(rcu::ReadablePtr<StubState>&& state, StubAny& stub)
    : state_{std::move(state)},
      stub_{stub}
{}

const ClientQos& StubHandle::GetClientQos() const { return state_->client_qos; }

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
