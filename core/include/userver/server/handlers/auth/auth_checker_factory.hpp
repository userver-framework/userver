#pragma once

/// @file userver/server/handlers/auth/auth_checker_factory.hpp
/// @brief Authorization factory registration and base classes.

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/handlers/handler_config.hpp>
#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

/// Base class for all the authorization factory checkers.
class AuthCheckerFactoryBase {
public:
    virtual ~AuthCheckerFactoryBase() = default;

    virtual AuthCheckerBasePtr MakeAuthChecker(const HandlerAuthConfig&) const = 0;
};

namespace impl {

using AuthCheckerFactoryFactory = utils::UniqueRef<AuthCheckerFactoryBase> (*)(const components::ComponentContext&);

void DoRegisterAuthCheckerFactory(std::string_view auth_type, AuthCheckerFactoryFactory factory);

utils::UniqueRef<AuthCheckerFactoryBase> MakeAuthCheckerFactory(
    std::string_view auth_type,
    const components::ComponentContext& context
);

std::vector<std::string> GetAllAuthTypes();

template <typename AuthCheckerFactory>
utils::UniqueRef<AuthCheckerFactoryBase> MakeAuthCheckerFactory(const components::ComponentContext& context) {
    return utils::MakeUniqueRef<AuthCheckerFactory>(context);
}

}  // namespace impl

/// @brief Function to call from main() to register an authorization checker.
///
/// @tparam AuthCheckerFactory must:
/// 1) inherit from @ref server::handlers::auth::AuthCheckerFactoryBase;
/// 2) have a constructor from `const components::ComponentContext&`.
/// @param auth_type Handler auth type to associate with the factory.
template <typename AuthCheckerFactory>
void RegisterAuthCheckerFactory(std::string_view auth_type) {
    if (auth_type.empty()) {
        throw std::invalid_argument("Auth checker type must not be empty; pass the handler auth type to register");
    }
    impl::DoRegisterAuthCheckerFactory(auth_type, &impl::MakeAuthCheckerFactory<AuthCheckerFactory>);
}

/// @brief Registers an authorization checker using its `kAuthType` member.
/// @tparam AuthCheckerFactory additionally must have a
/// `static constexpr std::string_view kAuthType = "..."` member.
template <typename AuthCheckerFactory>
void RegisterAuthCheckerFactory() {
    RegisterAuthCheckerFactory<AuthCheckerFactory>(std::string_view{AuthCheckerFactory::kAuthType});
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
