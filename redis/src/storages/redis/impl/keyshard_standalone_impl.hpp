#pragma once

#include <limits>

#include <storages/redis/impl/keyshard.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

class KeyShardStandalone : public KeyShard {
public:
    static constexpr char kName[] = "RedisStandalone";
    static constexpr std::size_t kUnknownShard = std::numeric_limits<std::size_t>::max();

    size_t ShardByKey(const std::string&) const override { return kUnknownShard; }
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
