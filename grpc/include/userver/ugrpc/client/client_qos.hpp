#pragma once

/// @file userver/ugrpc/client/client_qos.hpp
/// @brief @copybrief ugrpc::client::ClientQos

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/utils/default_dict.hpp>

#include <userver/ugrpc/client/qos.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief Maps full RPC names (`full.service.Name/MethodName`) to their QOS
/// configs. It is also possible to set `__default__` QOS that will be used
/// in place of missing entries.
using ClientQos = utils::DefaultDict<Qos>;

namespace impl {
extern const dynamic_config::Key<ClientQos> kNoClientQos;
}  // namespace impl

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
