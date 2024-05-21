#ifndef IMPL_KEYSHARD_STANDALONE_IMPL_HPP
#define IMPL_KEYSHARD_STANDALONE_IMPL_HPP

#include <userver/storages/redis/impl/keyshard.hpp>

#include <numeric>

USERVER_NAMESPACE_BEGIN

namespace redis {

class KeyShardStandalone : public KeyShard {
 public:
  static constexpr char kName[] = "RedisStandalone";
  static constexpr std::size_t kUnknownShard =
      std::numeric_limits<std::size_t>::max();

  size_t ShardByKey(const std::string&) const override { return kUnknownShard; }
  bool IsGenerateKeysForShardsEnabled() const override { return true; }
};

}

USERVER_NAMESPACE_END

#endif    /* IMPL_KEYSHARD_STANDALONE_IMPL_HPP */
