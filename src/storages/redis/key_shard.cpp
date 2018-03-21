#include "key_shard.hpp"

#include <algorithm>
#include <cassert>

#include <boost/crc.hpp>

namespace storages {
namespace redis {

void GetRedisKey(const std::string& key, size_t& key_start, size_t& key_len) {
  const char* str = key.data();
  size_t len = key.size();
  size_t start, end; /* start-end indexes of { and  } */

  key_start = 0;
  key_len = len;

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
  key_start = start + 1;
  key_len = end - start - 1;
}

size_t KeyShardCrc32::ShardByKey(const std::string& key) {
  assert(shard_count_ > 0);
  size_t start, len;
  GetRedisKey(key, start, len);

  return std::for_each(key.data() + start, key.data() + start + len,
                       boost::crc_32_type())() %
         shard_count_;
}

}  // namespace redis
}  // namespace storages
