#pragma once

/// @file userver/ydb/credentials.hpp
/// @brief @copybrief ydb::CredentialsProviderComponent

#include <memory>
#include <string>

#include <userver/components/component_base.hpp>
#include <userver/yaml_config/fwd.hpp>

namespace NYdb {
class ICredentialsProviderFactory;
}  // namespace NYdb

USERVER_NAMESPACE_BEGIN

namespace ydb {

// clang-format off

/// @ingroup userver_components
///
/// @brief Credentials provider component for creating custom credentials provider factory
///
/// Allows use custom credentials provider implementation
/// Required if `ydb::YdbComponent` comnponent config contains `databases.<dbname>.credentials`
///
/// see https://ydb.tech/docs/en/concepts/auth

// clang-format on

class CredentialsProviderComponent : public components::ComponentBase {
public:
    using components::ComponentBase::ComponentBase;

    /// @brief Create credentials provider factory
    ///
    /// @param credentials credentials config (`databases.<dbname>.credentials`
    /// from `ydb::YdbComponent` component config)
    virtual std::shared_ptr<NYdb::ICredentialsProviderFactory> CreateCredentialsProviderFactory(
        const yaml_config::YamlConfig& credentials
    ) const = 0;
};

}  // namespace ydb

USERVER_NAMESPACE_END
