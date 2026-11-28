#pragma once

/// @file userver/chaotic/openapi/client/auth_component.hpp
/// @brief @copybrief chaotic::openapi::client::AuthProviderComponentBase

#include <userver/chaotic/openapi/client/auth.hpp>
#include <userver/components/component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

/// @brief Base class of a component that provides an AuthProvider to generated clients.
///
/// A generated client takes the provider of a security scheme from the component named in its static config,
/// `auth-providers.<scheme-name>`.
class AuthProviderComponentBase : public components::ComponentBase {
public:
    using components::ComponentBase::ComponentBase;

    virtual AuthProvider& GetAuthProvider() = 0;
};

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
