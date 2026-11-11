#include <ugrpc/impl/event_engine/tcp_options.hpp>

#include <stdexcept>

#include <grpc/impl/channel_arg_names.h>
#include <gtest/gtest.h>
#include <src/core/lib/channel/channel_args.h>
#include <src/core/lib/event_engine/channel_args_endpoint_config.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {
namespace {

using Config = grpc_event_engine::experimental::ChannelArgsEndpointConfig;

TEST(GrpcEventEngineTcpOptions, InvalidLimitsAndReusePortAreRejected) {
    for (const int limit : {-2, 0}) {
        EXPECT_THROW(
            TcpOptionsFromEndpointConfig(Config{grpc_core::ChannelArgs{}.Set(kTcpMaxReadBufferSizeArg, limit)}),
            std::invalid_argument
        );
    }
    EXPECT_THROW(
        TcpOptionsFromEndpointConfig(Config{grpc_core::ChannelArgs{}.Set(kTcpMaxReadBufferSizeArg, "8192")}),
        std::invalid_argument
    );
    EXPECT_THROW(
        TcpOptionsFromEndpointConfig(Config{grpc_core::ChannelArgs{}.Set(GRPC_ARG_ALLOW_REUSEPORT, 2)}),
        std::invalid_argument
    );
}

}  // namespace
}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END
