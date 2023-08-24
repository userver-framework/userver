#include "keys_for_shards.hpp"

#include <cassert>
#include <sstream>
#include <stdexcept>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

KeysForShards::KeysForShards(
    size_t shard_count,
    const std::function<size_t(const std::string&)>& shard_by_key,
    size_t max_len) {
  keys_.resize(shard_count);
  size_t need = shard_count;
  for (size_t len = 1; len <= max_len; len++) {
    std::string key_buf(len, ' ');
    GenerateLexMinKeysForShards(0, len, shard_by_key, need, key_buf, keys_);
    if (!need) break;
  }
  if (need) {
    for (size_t i = 0; i < keys_.size(); i++) {
      if (keys_[i].empty())
        throw std::runtime_error(
            "failed to generate key with length<=" + std::to_string(max_len) +
            " for shard=" + std::to_string(i));
    }
    throw std::logic_error("need > 0, but all keys are not empty");
  }

  LOG_DEBUG() << "keys for shards: " << KeysToDebugString();
}

const std::string& KeysForShards::GetAnyKeyForShard(size_t shard_idx) const {
  return keys_.at(shard_idx);
}

void KeysForShards::GenerateLexMinKeysForShards(
    size_t pos, size_t len,
    const std::function<size_t(const std::string&)>& shard_by_key, size_t& need,
    std::string& key_buf, std::vector<std::string>& keys) {
  if (pos == len) {
    size_t shard = shard_by_key(key_buf);
    if (keys.at(shard).empty()) {
      keys[shard] = key_buf;
      UASSERT(need > 0);
      need--;
    }
    return;
  }
  for (char c = 'a'; c <= 'z'; c++) {
    key_buf[pos] = c;
    GenerateLexMinKeysForShards(pos + 1, len, shard_by_key, need, key_buf,
                                keys);
    if (!need) return;
  }
}

std::string KeysForShards::KeysToDebugString() const {
  std::ostringstream os;
  os << '[';
  for (size_t i = 0; i < keys_.size(); i++) {
    if (i) os << ", ";
    os << keys_[i];
  }
  os << ']';
  return os.str();
}

}  // namespace redis

USERVER_NAMESPACE_END
