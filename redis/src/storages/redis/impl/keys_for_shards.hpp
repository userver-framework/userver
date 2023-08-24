#pragma once

#include <functional>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace redis {

class KeysForShards {
 public:
  KeysForShards(size_t shard_count,
                const std::function<size_t(const std::string&)>& shard_by_key,
                size_t max_len = 4);

  // Returns a non-empty key of the minimum length consisting of lowercase
  // letters for a given shard.
  // Selects the lexicographically smallest of keys of the same length.
  const std::string& GetAnyKeyForShard(size_t shard_idx) const;

 private:
  static void GenerateLexMinKeysForShards(
      size_t pos, size_t len,
      const std::function<size_t(const std::string&)>& shard_by_key,
      size_t& need, std::string& key_buf, std::vector<std::string>& keys);

  std::string KeysToDebugString() const;

  std::vector<std::string> keys_;
};

}  // namespace redis

USERVER_NAMESPACE_END
