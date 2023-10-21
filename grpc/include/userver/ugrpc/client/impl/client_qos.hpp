#pragma once

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/ugrpc/client/qos.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

using ClientQos = dynamic_config::ValueDict<Qos>;

extern const dynamic_config::Key<ClientQos> kNoClientQos;

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
