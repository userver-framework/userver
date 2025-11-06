#pragma once

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class ClientData;

class ClientDataAccessor final {
public:
    template <typename Client>
    static const ClientData& GetClientData(const Client& client) {
        return *client.client_data_;
    }
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
