#pragma once

#include <string>

namespace storages {
namespace redis {

class KeyShard {
 public:
  virtual ~KeyShard() {}
  virtual size_t ShardByKey(const std::string& key) = 0;
};

class KeyShardZero : public KeyShard {
 public:
  virtual ~KeyShardZero() {}

  virtual size_t ShardByKey(const std::string&) override { return 0; }
};

class KeyShardCrc32 : public KeyShard {
 public:
  KeyShardCrc32(size_t shard_count) : shard_count_(shard_count) {}
  virtual ~KeyShardCrc32() {}

  virtual size_t ShardByKey(const std::string& key) override;

 private:
  size_t shard_count_ = 0;
};

void GetRedisKey(const std::string& key, size_t& key_start, size_t& key_len);

}  // namespace redis
}  // namespace storages
