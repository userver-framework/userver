#pragma once

#include <grpcpp/channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/channel_arguments.h>

#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class ChannelFactory final {
public:
    ChannelFactory(
        engine::TaskProcessor& blocking_task_processor,
        std::string endpoint,
        std::shared_ptr<grpc::ChannelCredentials> credentials
    );

    ChannelFactory(ChannelFactory&&) noexcept = default;
    ChannelFactory(const ChannelFactory&) = delete;
    ChannelFactory& operator=(ChannelFactory&&) = delete;
    ChannelFactory& operator=(const ChannelFactory&) = delete;

    std::shared_ptr<grpc::Channel> CreateChannel(const grpc::ChannelArguments& channel_args) const;

private:
    engine::TaskProcessor& blocking_task_processor_;
    grpc::string endpoint_;
    std::shared_ptr<grpc::ChannelCredentials> credentials_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
