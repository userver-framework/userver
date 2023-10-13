#include <userver/ugrpc/client/impl/client_qos.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

ClientQos ParseNoClientQos(const dynamic_config::DocsMap&) {
  return ClientQos{{"__default__", {/*timeout=*/std::nullopt}}};
}

constexpr dynamic_config::Key<ParseNoClientQos> kNoClientQos;

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
