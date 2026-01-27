#pragma once

#include <grpcpp/channel.h>

#include <userver/utils/fixed_array.hpp>

#include <userver/ugrpc/client/impl/stub_any.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class StubPool final {
public:
    StubPool() = default;

    StubPool(utils::FixedArray<std::shared_ptr<grpc::Channel>>&& channels, utils::FixedArray<StubAny>&& stubs);

    std::size_t Size() const { return stubs_.size(); }

    StubAny& NextStub() const;

    const utils::FixedArray<std::shared_ptr<grpc::Channel>>& GetChannels() const { return channels_; }

    const utils::FixedArray<StubAny>& GetStubs() const { return stubs_; }

private:
    utils::FixedArray<std::shared_ptr<grpc::Channel>> channels_;
    mutable utils::FixedArray<StubAny> stubs_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
