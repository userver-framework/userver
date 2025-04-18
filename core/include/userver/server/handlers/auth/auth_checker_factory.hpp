#pragma once

/// @file userver/server/handlers/auth/auth_checker_factory.hpp
/// @brief Authorization factory registration and base classes.

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

utils::UniqueRef<AuthCheckerFactoryBase>
MakeAuthCheckerFactory(std::string_view auth_type, const components::ComponentContext& context);

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
/// 2) have a constructor from `const components::ComponentContext&`;
/// 3) have `static constexpr std::string_view kAuthType = "..."` member.
template <typename AuthCheckerFactory>
void RegisterAuthCheckerFactory() {
    const auto auth_type = std::string_view{AuthCheckerFactory::kAuthType};
    impl::DoRegisterAuthCheckerFactory(auth_type, &impl::MakeAuthCheckerFactory<AuthCheckerFactory>);
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
