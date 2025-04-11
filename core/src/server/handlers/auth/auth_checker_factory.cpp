#include <userver/server/handlers/auth/auth_checker_factory.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <fmt/format.h>
#include <boost/range/adaptor/map.hpp>

#include <userver/utils/algo.hpp>
#include <userver/utils/impl/transparent_hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {
namespace {

class AuthCheckerFactories final {
public:
    void RegisterFactory(std::string auth_type, impl::AuthCheckerFactoryFactory factory) {
        auto res = factories_.emplace(std::move(auth_type), factory);
        if (!res.second) {
            throw std::runtime_error(fmt::format("can't register auth checker with type {}", res.first->first));
        }
    }

    impl::AuthCheckerFactoryFactory GetFactory(std::string_view auth_type) const {
        if (auto* const factory = utils::impl::FindTransparentOrNullptr(factories_, auth_type)) {
            return *factory;
        }
        throw std::runtime_error(fmt::format("unknown type of auth checker: {}", auth_type));
    }

    std::vector<std::string> GetAllAuthTypes() const {
        return utils::AsContainer<std::vector<std::string>>(factories_ | boost::adaptors::map_keys);
    }

private:
    utils::impl::TransparentMap<std::string, impl::AuthCheckerFactoryFactory> factories_;
};

AuthCheckerFactories& GetAuthCheckerFactories() {
    static AuthCheckerFactories auth_checker_factories;
    return auth_checker_factories;
}

}  // namespace

namespace impl {

void DoRegisterAuthCheckerFactory(std::string_view auth_type, AuthCheckerFactoryFactory factory) {
    GetAuthCheckerFactories().RegisterFactory(std::string{auth_type}, factory);
}

utils::UniqueRef<AuthCheckerFactoryBase>
MakeAuthCheckerFactory(std::string_view auth_type, const components::ComponentContext& context) {
    const auto factory_factory = GetAuthCheckerFactories().GetFactory(auth_type);
    return factory_factory(context);
}

std::vector<std::string> GetAllAuthTypes() { return GetAuthCheckerFactories().GetAllAuthTypes(); }

}  // namespace impl

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
