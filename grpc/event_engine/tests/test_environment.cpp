#include <grpc/grpc.h>
#include <gtest/gtest.h>

#include <userver/ugrpc/impl/event_engine.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class EventEngineEnvironment final : public ::testing::Environment {
public:
    void SetUp() override {
        ugrpc::impl::event_engine::EnsureRuntimeConfigured();
        ugrpc::impl::event_engine::RegisterEventEngineArgument();
        grpc_init();
    }

    void TearDown() override { grpc_shutdown(); }
};

[[maybe_unused]] const auto* const environment = ::testing::AddGlobalTestEnvironment(new EventEngineEnvironment);

}  // namespace

USERVER_NAMESPACE_END
