#pragma once
#include <userver/storages/redis/impl/keyshard.hpp>

#include <optional>
#include <utils/encoding.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

class KeyShardZero : public KeyShard {
 public:
  static constexpr char kName[] = "KeyShardZero";

  size_t ShardByKey(const std::string&) const override { return 0; }
  bool IsGenerateKeysForShardsEnabled() const override { return false; }
};

class KeyShardCrc32 : public KeyShard {
 public:
  KeyShardCrc32(size_t shard_count) : shard_count_(shard_count) {}

  static constexpr char kName[] = "KeyShardCrc32";

  size_t ShardByKey(const std::string& key) const override;
  bool IsGenerateKeysForShardsEnabled() const override { return true; }

 private:
  size_t shard_count_ = 0;
};

class KeyShardTaximeterCrc32 : public KeyShard {
 public:
  KeyShardTaximeterCrc32(size_t shard_count);

  size_t ShardByKey(const std::string& key) const override;
  bool IsGenerateKeysForShardsEnabled() const override { return true; }

 private:
  static bool NeedConvertEncoding(const std::string& key, size_t start,
                                  size_t len);
  size_t shard_count_ = 0;
  utils::encoding::Converter converter_;
};

class KeyShardGpsStorageDriver : public redis::KeyShard {
 public:
  KeyShardGpsStorageDriver(size_t shard_count) : shard_count_(shard_count) {}

  size_t ShardByKey(const std::string& key) const override;
  bool IsGenerateKeysForShardsEnabled() const override { return false; }

 private:
  static std::optional<std::string> Parse(const std::string& s);

  size_t shard_count_ = 0;
};

inline constexpr char kRedisCluster[] = "RedisCluster";

bool IsClusterStrategy(const std::string& type);

}  // namespace redis

USERVER_NAMESPACE_END
