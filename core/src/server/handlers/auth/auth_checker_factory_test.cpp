#include <userver/server/handlers/auth/auth_checker_factory.hpp>

#include <string_view>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

namespace {

class TestFactory final : public AuthCheckerFactoryBase {
public:
    explicit TestFactory(const components::ComponentContext&) {}

    AuthCheckerBasePtr MakeAuthChecker(const HandlerAuthConfig&) const override { return {}; }
};

UTEST(AuthCheckerFactory, EmptyExplicitTypeIsRejected) {
    EXPECT_THROW(RegisterAuthCheckerFactory<TestFactory>(std::string_view{}), std::invalid_argument);
}

}  // namespace

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
