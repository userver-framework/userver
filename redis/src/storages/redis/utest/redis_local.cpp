#include <userver/storages/redis/utest/redis_local.hpp>

#include <storages/redis/utest/impl/redis_connection_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::utest {

namespace impl {

class RedisLocalImpl : public RedisConnectionState {
 public:
  void FlushDb() {
    GetSentinel()->MakeRequest({"flushdb"}, "none", true).Get();
  }
};

}  // namespace impl

RedisLocal::RedisLocal() : impl_{std::make_unique<impl::RedisLocalImpl>()} {}

RedisLocal::~RedisLocal() = default;

ClientPtr RedisLocal::GetClient() const { return impl_->GetClient(); }

SubscribeClientPtr RedisLocal::GetSubscribeClient() const {
  return impl_->GetSubscribeClient();
}

void RedisLocal::FlushDb() { impl_->FlushDb(); }

}  // namespace storages::redis::utest

USERVER_NAMESPACE_END
