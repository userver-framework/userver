#include <userver/ugrpc/client/impl/stub_pool.hpp>

#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

StubAny& StubPool::NextStub() const { return stubs_[utils::RandRange(stubs_.size())]; }

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
