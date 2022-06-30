#include "keyshard_impl.hpp"

#include <algorithm>
#include <cassert>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/crc.hpp>
#include <boost/range/algorithm/for_each.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {
namespace {

const std::string kRawKeyEncoding = "UTF-8";
const std::string kTaximeterCrcKeyEncoding = "WINDOWS-1252//TRANSLIT";

}  // namespace

void GetRedisKey(const std::string& key, size_t* key_start, size_t* key_len) {
  // see https://redis.io/topics/cluster-spec

  const char* str = key.data();
  size_t len = key.size();
  size_t start = 0;  // start-end indexes
  size_t end = 0;    // of '{' and '}'

  *key_start = 0;
  *key_len = len;

  /* Search the first occurrence of '{'. */
  for (start = 0; start < len; start++)
    if (str[start] == '{') break;

  /* No '{' ? Hash the whole key. This is the base case. */
  if (start == len) return;

  /* '{' found? Check if we have the corresponding '}'. */
  for (end = start + 1; end < len; end++)
    if (str[end] == '}') break;

  /* No '}' or nothing between {} ? Hash the whole key. */
  if (end == len || end == start + 1) return;

  /* If we are here there is both a { and a  } on its right. Hash
   * what is in the middle between { and  }. */
  *key_start = start + 1;
  *key_len = end - start - 1;
}

KeyShardTaximeterCrc32::KeyShardTaximeterCrc32(size_t shard_count)
    : shard_count_(shard_count),
      converter_(kRawKeyEncoding, kTaximeterCrcKeyEncoding) {}

size_t KeyShardCrc32::ShardByKey(const std::string& key) const {
  UASSERT(shard_count_ > 0);
  size_t start = 0;
  size_t len = 0;
  GetRedisKey(key, &start, &len);

  return std::for_each(key.data() + start, key.data() + start + len,
                       boost::crc_32_type())() %
         shard_count_;
}

bool KeyShardTaximeterCrc32::NeedConvertEncoding(const std::string& key,
                                                 size_t start, size_t len) {
  for (size_t i = 0; i < len; i++)
    if ((key[start + i] & 0x80) != 0) return true;
  return false;
}

size_t KeyShardTaximeterCrc32::ShardByKey(const std::string& key) const {
  UASSERT(shard_count_ > 0);
  size_t start = 0;
  size_t len = 0;
  GetRedisKey(key, &start, &len);

  std::vector<char> converted;
  if (NeedConvertEncoding(key, start, len) &&
      converter_.Convert(key.data() + start, len, converted))
    return std::for_each(converted.begin(), converted.end(),
                         boost::crc_32_type())() %
           shard_count_;
  else
    return std::for_each(key.data() + start, key.data() + start + len,
                         boost::crc_32_type())() %
           shard_count_;
}

size_t KeyShardGpsStorageDriver::ShardByKey(const std::string& key) const {
  const auto path = Parse(key);
  const auto& driver_id = path.value_or(key);
  boost::crc_32_type crc{};
  return boost::for_each(driver_id, crc)() % shard_count_;
}

std::optional<std::string> KeyShardGpsStorageDriver::Parse(
    const std::string& s) {
  // TODO: this copies geotracks implementation
  // need use sime single function for this, but unsure how to get dependencies
  // right
  std::vector<std::string> parts;
  boost::algorithm::split(parts, s, [](char c) { return c == '/'; });
  if (parts.size() != 5) return std::nullopt;
  return parts[2];  // data/db/driver_id/data/bucket
}

std::unique_ptr<redis::KeyShard> KeyShardFactory::operator()(size_t nshards) {
  LOG_TRACE() << "Create KeyShard with type '" << type_ << '\'';
  if (type_ == "KeyShardGpsStorageDriver")
    return std::make_unique<redis::KeyShardGpsStorageDriver>(nshards);
  if (type_ == "KeyShardTaximeterCrc32")
    return std::make_unique<redis::KeyShardTaximeterCrc32>(nshards);
  if (type_ == KeyShardCrc32::kName)
    return std::make_unique<redis::KeyShardCrc32>(nshards);
  if (type_ == kRedisCluster) return nullptr;

  return std::make_unique<redis::KeyShardTaximeterCrc32>(nshards);
}

bool IsClusterStrategy(const std::string& type) {
  return type == kRedisCluster;
}

}  // namespace redis

USERVER_NAMESPACE_END
