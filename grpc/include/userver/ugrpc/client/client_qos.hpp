#pragma once

/// @file userver/ugrpc/client/client_qos.hpp
/// @brief @copybrief ugrpc::client::ClientQos

#include <userver/dynamic_config/fwd.hpp>
#include <userver/utils/default_dict.hpp>

#include <userver/ugrpc/client/qos.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

struct GlobalQos final {};

bool operator==(const GlobalQos& lhs, const GlobalQos& rhs) noexcept;

GlobalQos Parse(const formats::json::Value& value, formats::parse::To<GlobalQos>);

formats::json::Value Serialize(const GlobalQos&, formats::serialize::To<formats::json::Value>);

struct ClientQos {
    /// @brief Maps full RPC names (`full.service.Name/MethodName`) to their QOS
    /// configs. It is also possible to set `__default__` QOS that will be used
    /// in place of missing entries.
    utils::DefaultDict<Qos> methods;

    /// @brief Properties that could not be specified per-method.
    std::optional<GlobalQos> global;
};

bool operator==(const ClientQos& lhs, const ClientQos& rhs) noexcept;

ClientQos Parse(const formats::json::Value& value, formats::parse::To<ClientQos>);

formats::json::Value Serialize(const ClientQos& client_qos, formats::serialize::To<formats::json::Value>);

namespace impl {
extern const dynamic_config::Key<ClientQos> kNoClientQos;
}  // namespace impl

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
