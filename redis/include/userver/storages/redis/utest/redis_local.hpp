#pragma once

/// @file userver/storages/redis/utest/redis_local.hpp
/// @brief @copybrief storages::redis::RedisLocal

#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/subscribe_client.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::utest {

namespace impl {
class RedisLocalImpl;
}  // namespace impl

/// @brief Redis local class
///
/// Provide access to localhost Redis
class RedisLocal {
public:
    RedisLocal();
    ~RedisLocal();

    /// Get client
    ClientPtr GetClient() const;

    /// Get subscribe client
    SubscribeClientPtr GetSubscribeClient() const;

    /// call `flushdb` command
    void FlushDb();

private:
    std::unique_ptr<impl::RedisLocalImpl> impl_;
};

}  // namespace storages::redis::utest

USERVER_NAMESPACE_END
