#pragma once

#include <optional>

#include <storages/redis/impl/keyshard.hpp>
#include <utils/encoding.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

class KeyShardZero : public KeyShard {
public:
    static constexpr char kName[] = "KeyShardZero";

    size_t ShardByKey(const std::string&) const override { return 0; }
};

class KeyShardCrc32 : public KeyShard {
public:
    KeyShardCrc32(size_t shard_count) : shard_count_(shard_count) {}

    static constexpr char kName[] = "KeyShardCrc32";

    size_t ShardByKey(const std::string& key) const override;

private:
    size_t shard_count_ = 0;
};

class KeyShardTaximeterCrc32 : public KeyShard {
public:
    KeyShardTaximeterCrc32(size_t shard_count);

    size_t ShardByKey(const std::string& key) const override;

private:
    static bool NeedConvertEncoding(const std::string& key, size_t start, size_t len);
    size_t shard_count_ = 0;
    utils::encoding::Converter converter_;
};

class KeyShardGpsStorageDriver : public KeyShard {
public:
    KeyShardGpsStorageDriver(size_t shard_count) : shard_count_(shard_count) {}

    size_t ShardByKey(const std::string& key) const override;

private:
    static std::optional<std::string> Parse(const std::string& s);

    size_t shard_count_ = 0;
};

inline constexpr char kRedisCluster[] = "RedisCluster";

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
