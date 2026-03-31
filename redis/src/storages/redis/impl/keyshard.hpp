#pragma once

#include <memory>
#include <string>

#include <userver/storages/redis/sharding_strategies.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

void GetRedisKey(const std::string& key, size_t* key_start, size_t* key_len);

class KeyShard {
public:
    virtual ~KeyShard() = default;
    virtual size_t ShardByKey(const std::string& key) const = 0;
};

class KeyShardFactory {
    ShardingStrategy sharding_strategy_;

public:
    KeyShardFactory(ShardingStrategy sharding_strategy)
        : sharding_strategy_(sharding_strategy)
    {}
    std::unique_ptr<KeyShard> operator()(size_t nshards) const;
    bool IsClusterStrategy() const;

    std::string_view GetShardingStrategyAsString() const { return ToStringView(sharding_strategy_); }
    ShardingStrategy GetShardingStrategy() const { return sharding_strategy_; }
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
