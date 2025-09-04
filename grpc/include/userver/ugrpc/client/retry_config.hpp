#pragma once

/// @file userver/ugrpc/client/retry_config.hpp
/// @brief @copybrief ugrpc::client::RetryConfig

#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// Retry configuration for outgoing RPCs
struct RetryConfig final {
    /// The maximum number of RPC attempts, including the original attempt
    int attempts{1};
};

RetryConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<RetryConfig>);

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
