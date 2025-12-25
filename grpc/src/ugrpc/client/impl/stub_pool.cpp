#include <userver/ugrpc/client/impl/stub_pool.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

StubPool::StubPool(utils::FixedArray<std::shared_ptr<grpc::Channel>>&& channels, utils::FixedArray<StubAny>&& stubs)
    : channels_{std::move(channels)},
      stubs_{std::move(stubs)}
{}

StubAny& StubPool::NextStub() const {
    UINVARIANT(!stubs_.empty(), "stubs should not be empty");
    if (1 == stubs_.size()) {
        return stubs_[0];
    }
    return stubs_[utils::RandRange(stubs_.size())];
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
