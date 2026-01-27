#pragma once

/// @file userver/ugrpc/client/generic_options.hpp
/// @brief @copybrief ugrpc::client::GenericOptions

#include <optional>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

struct GenericOptions {
    /// If non-`nullopt`, metrics are accounted for specified fake call name.
    /// If `nullopt`, writes a set of metrics per real call name.
    /// If the microservice serves as a proxy and has untrusted clients, it is
    /// a good idea to have this option set to non-`nullopt` to avoid
    /// the situations where an upstream client can spam various RPCs with
    /// non-existent names, which leads to this microservice spamming RPCs
    /// with non-existent names, which leads to creating storage for infinite
    /// metrics and causes OOM.
    /// The default is to specify `"Generic/Generic"` fake call name.
    std::optional<std::string_view> metrics_call_name{"Generic/Generic"};
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
