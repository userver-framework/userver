#include <userver/ugrpc/client/impl/client_qos.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

const dynamic_config::Key<ClientQos> kNoClientQos{
    dynamic_config::ConstantConfig{},
    ClientQos{{"__default__", {/*timeout=*/std::nullopt}}},
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
