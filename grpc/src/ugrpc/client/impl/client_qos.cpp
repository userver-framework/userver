#include <userver/ugrpc/client/client_qos.hpp>

#include <userver/dynamic_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

const dynamic_config::Key<ClientQos> kNoClientQos{
    dynamic_config::ConstantConfig{},
    ClientQos{{{"__default__", Qos{/*attempts=*/std::nullopt, /*timeout=*/std::nullopt}}}, std::nullopt},
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
