#pragma once

#include <grpcpp/channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/channel_arguments.h>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/ugrpc/client/auth_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class ChannelFactory final {
public:
    ChannelFactory(
        engine::TaskProcessor& blocking_task_processor,
        std::shared_ptr<grpc::ChannelCredentials> credentials,
        AuthType auth_type
    );

    ChannelFactory(ChannelFactory&&) noexcept = default;
    ChannelFactory(const ChannelFactory&) = delete;
    ChannelFactory& operator=(ChannelFactory&&) = delete;
    ChannelFactory& operator=(const ChannelFactory&) = delete;

    AuthType GetAuthType() const { return auth_type_; }

    std::shared_ptr<grpc::Channel> CreateChannel(std::string_view target, const grpc::ChannelArguments& channel_args)
        const;

private:
    engine::TaskProcessor& blocking_task_processor_;
    std::shared_ptr<grpc::ChannelCredentials> credentials_;
    AuthType auth_type_{};
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
