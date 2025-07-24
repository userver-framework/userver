#pragma once

#include <algorithm>
#include <array>

#include <gmock/gmock-nice-strict.h>

#include <userver/ugrpc/client/middlewares/fwd.hpp>
#include <userver/ugrpc/server/middlewares/fwd.hpp>

#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests {

template <class MiddlewareType, class ServiceType, class ClientType, std::size_t N>
class MiddlewaresFixture : public ugrpc::tests::ServiceFixtureBase {
public:
    using MiddlewareMock = ::testing::NiceMock<MiddlewareType>;

    static_assert(std::is_base_of_v<ugrpc::server::MiddlewareBase, MiddlewareType> || std::is_base_of_v<ugrpc::client::MiddlewareBase, MiddlewareType>);

protected:
    MiddlewaresFixture() {
        std::generate(middlewares_.begin(), middlewares_.end(), [] { return std::make_shared<MiddlewareMock>(); });

        if constexpr (std::is_base_of_v<ugrpc::server::MiddlewareBase, MiddlewareType>) {
            SetServerMiddlewares({middlewares_.begin(), middlewares_.end()});
        } else {
            SetClientMiddlewares({middlewares_.begin(), middlewares_.end()});
        }
        RegisterService(service_);
        StartServer();
        client_.emplace(MakeClient<ClientType>());
    }

    ~MiddlewaresFixture() override {
        client_.reset();
        StopServer();
    }

    ServiceType& Service() { return service_; }

    ClientType& Client() { return client_.value(); }

    const MiddlewareMock& Middleware(std::size_t index) const { return *middlewares_[index]; }

private:
    ServiceType service_;
    std::optional<ClientType> client_;
    std::array<std::shared_ptr<const MiddlewareMock>, N> middlewares_;
};

}  // namespace tests

USERVER_NAMESPACE_END
