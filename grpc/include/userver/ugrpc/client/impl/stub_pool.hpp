#pragma once

#include <grpcpp/channel.h>

#include <userver/utils/fixed_array.hpp>

#include <userver/ugrpc/client/impl/channel_factory.hpp>
#include <userver/ugrpc/client/impl/stub_any.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class StubPool final {
public:
    template <typename Stub>
    static StubPool
    Create(std::size_t size, const ChannelFactory& channel_factory, const grpc::ChannelArguments& channel_args) {
        auto channels = utils::GenerateFixedArray(size, [&channel_factory, &channel_args](std::size_t) {
            return channel_factory.CreateChannel(channel_args);
        });
        auto stubs = utils::GenerateFixedArray(channels.size(), [&channels](std::size_t index) {
            return MakeStub<Stub>(channels[index]);
        });
        return StubPool{std::move(channels), std::move(stubs)};
    }

    StubPool() = default;

    std::size_t Size() const { return stubs_.size(); }

    StubAny& NextStub() const;

    const utils::FixedArray<std::shared_ptr<grpc::Channel>>& GetChannels() const { return channels_; }

    const utils::FixedArray<StubAny>& GetStubs() const { return stubs_; }

private:
    StubPool(utils::FixedArray<std::shared_ptr<grpc::Channel>>&& channels, utils::FixedArray<StubAny>&& stubs)
        : channels_{std::move(channels)}, stubs_{std::move(stubs)} {}

    utils::FixedArray<std::shared_ptr<grpc::Channel>> channels_;

    mutable utils::FixedArray<StubAny> stubs_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
