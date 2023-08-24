#pragma once

#include <userver/dynamic_config/value.hpp>
#include <userver/ugrpc/client/qos.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

using ClientQos = dynamic_config::ValueDict<Qos>;

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
