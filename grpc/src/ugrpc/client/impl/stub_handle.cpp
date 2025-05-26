#include <userver/ugrpc/client/impl/stub_handle.hpp>

#include <userver/ugrpc/client/impl/client_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

const ClientQos& StubHandle::GetClientQos() const { return state_->client_qos; }

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
