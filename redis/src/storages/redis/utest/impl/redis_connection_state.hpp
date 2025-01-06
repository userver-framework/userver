#pragma once

#include <storages/redis/impl/thread_pools.hpp>

#include <storages/redis/client_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/subscribe_sentinel.hpp>
#include <storages/redis/subscribe_client_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::utest::impl {

class RedisConnectionState {
public:
    ClientPtr GetClient() const { return client_; }

    SubscribeClientPtr GetSubscribeClient() const { return subscribe_client_; }

protected:
    RedisConnectionState();

    class InClusterMode {};
    explicit RedisConnectionState(InClusterMode);

    std::shared_ptr<storages::redis::impl::Sentinel> GetSentinel() const { return sentinel_; }

private:
    std::shared_ptr<storages::redis::impl::ThreadPools> thread_pools_;
    std::shared_ptr<storages::redis::impl::Sentinel> sentinel_;
    ClientPtr client_;
    std::shared_ptr<storages::redis::impl::SubscribeSentinel> subscribe_sentinel_;
    SubscribeClientPtr subscribe_client_;
};

struct RedisClusterConnectionState : public RedisConnectionState {
    RedisClusterConnectionState() : RedisConnectionState(InClusterMode{}) {}
};

}  // namespace storages::redis::utest::impl

USERVER_NAMESPACE_END
