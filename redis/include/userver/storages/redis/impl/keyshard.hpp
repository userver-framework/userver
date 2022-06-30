#pragma once

#include <memory>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace redis {

void GetRedisKey(const std::string& key, size_t* key_start, size_t* key_len);

class KeyShard {
 public:
  virtual ~KeyShard() = default;
  virtual size_t ShardByKey(const std::string& key) const = 0;
  virtual bool IsGenerateKeysForShardsEnabled() const = 0;
};

class KeyShardFactory {
  std::string type_;

 public:
  KeyShardFactory(const std::string& type) : type_(type) {}
  std::unique_ptr<redis::KeyShard> operator()(size_t nshards);
};

enum class PubShard {
  kZeroShard,
  kRoundRobin,
};

}  // namespace redis

USERVER_NAMESPACE_END
