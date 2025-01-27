#pragma once

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/channel_arguments.h>

#include <userver/engine/async.hpp>

#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class ChannelFactory final {
public:
    ChannelFactory(
        engine::TaskProcessor& blocking_task_processor,
        const std::string& endpoint,
        std::shared_ptr<grpc::ChannelCredentials> credentials,
        const grpc::ChannelArguments& channel_args
    )
        : blocking_task_processor_(blocking_task_processor),
          endpoint_{ugrpc::impl::ToGrpcString(endpoint)},
          credentials_{std::move(credentials)},
          channel_args_{channel_args} {}

    std::shared_ptr<grpc::Channel> CreateChannel() const {
        return engine::AsyncNoSpan(
                   blocking_task_processor_,
                   grpc::CreateCustomChannel,
                   std::ref(endpoint_),
                   std::ref(credentials_),
                   std::ref(channel_args_)
        )
            .Get();
    }

private:
    engine::TaskProcessor& blocking_task_processor_;
    grpc::string endpoint_;
    std::shared_ptr<grpc::ChannelCredentials> credentials_;
    const grpc::ChannelArguments& channel_args_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
