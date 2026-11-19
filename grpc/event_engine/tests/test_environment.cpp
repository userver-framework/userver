#include <optional>

#include <gtest/gtest.h>

#include <userver/ugrpc/impl/grpc_init.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class EventEngineEnvironment final : public ::testing::Environment {
public:
    void SetUp() override { grpc_init_.emplace(); }

    void TearDown() override { grpc_init_.reset(); }

private:
    std::optional<ugrpc::impl::event_engine::GrpcInit> grpc_init_;
};

[[maybe_unused]] const auto* const environment = ::testing::AddGlobalTestEnvironment(new EventEngineEnvironment);

}  // namespace

USERVER_NAMESPACE_END
