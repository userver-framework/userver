#pragma once

/// @file storages/redis/client.hpp
/// @brief @copybrief storages::redis::Client

#include <memory>
#include <string>

#include <redis/base.hpp>
#include <redis/command_options.hpp>

#include <storages/redis/request.hpp>

namespace storages {
namespace redis {

using CommandControl = ::redis::CommandControl;
using RangeOptions = ::redis::RangeOptions;
using ZaddOptions = ::redis::ZaddOptions;

enum class PubShard {
  kZeroShard,
  kRoundRobin,
};

/// @class Client
class Client {
 public:
  virtual ~Client() = default;

  virtual size_t ShardsCount() const = 0;

  virtual size_t ShardByKey(const std::string& key) const = 0;

  virtual const std::string& GetAnyKeyForShard(size_t shard_idx) const = 0;

  // redis commands:

  virtual RequestAppend Append(const std::string& key, std::string value,
                               const CommandControl& command_control) = 0;

  virtual RequestDel Del(const std::string& key,
                         const CommandControl& command_control) = 0;

  virtual RequestGet Get(const std::string& key,
                         const CommandControl& command_control) = 0;

  virtual RequestHdel Hdel(const std::string& key, const std::string& field,
                           const CommandControl& command_control) = 0;

  virtual RequestHget Hget(const std::string& key, const std::string& field,
                           const CommandControl& command_control) = 0;

  virtual RequestHgetall Hgetall(const std::string& key,
                                 const CommandControl& command_control) = 0;

  virtual RequestHmset Hmset(
      const std::string& key,
      const std::vector<std::pair<std::string, std::string>>& field_val,
      const CommandControl& command_control) = 0;

  virtual RequestHset Hset(const std::string& key, const std::string& field,
                           const std::string& value,
                           const CommandControl& command_control) = 0;

  virtual RequestKeys Keys(std::string keys_pattern, size_t shard,
                           const CommandControl& command_control) = 0;

  virtual RequestMget Mget(std::vector<std::string> keys,
                           const CommandControl& command_control) = 0;

  virtual RequestPing Ping(size_t shard,
                           const CommandControl& command_control) = 0;

  virtual RequestPingMessage Ping(size_t shard, std::string message,
                                  const CommandControl& command_control) = 0;

  virtual RequestPublish Publish(const std::string& channel,
                                 std::string message,
                                 const CommandControl& command_control) = 0;

  virtual RequestPublish Publish(const std::string& channel,
                                 std::string message,
                                 const CommandControl& command_control,
                                 PubShard policy) = 0;

  virtual RequestSet Set(const std::string& key, std::string value,
                         const CommandControl& command_control) = 0;

  virtual RequestSetex Setex(const std::string& key, int64_t seconds,
                             std::string value,
                             const CommandControl& command_control) = 0;

  virtual RequestStrlen Strlen(const std::string& key,
                               const CommandControl& command_control) = 0;

  virtual RequestTtl Ttl(const std::string& key,
                         const CommandControl& command_control) = 0;

  virtual RequestType Type(const std::string& key,
                           const CommandControl& command_control) = 0;

  virtual RequestZadd Zadd(const std::string& key, double score,
                           const std::string& member,
                           const CommandControl& command_control) = 0;

  virtual RequestZadd Zadd(const std::string& key, double score,
                           const std::string& member,
                           const ZaddOptions& options,
                           const CommandControl& command_control) = 0;

  virtual RequestZrangebyscore Zrangebyscore(
      const std::string& key, double min, double max,
      const RangeOptions& range_options,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscore Zrangebyscore(
      const std::string& key, const std::string& min, const std::string& max,
      const RangeOptions& range_options,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      const std::string& key, double min, double max,
      const RangeOptions& range_options,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      const std::string& key, const std::string& min, const std::string& max,
      const RangeOptions& range_options,
      const CommandControl& command_control) = 0;

  virtual RequestZrem Zrem(const std::string& key, const std::string& member,
                           const CommandControl& command_control) = 0;

  virtual RequestZscore Zscore(const std::string& key,
                               const std::string& member,
                               const CommandControl& command_control) = 0;
};

using ClientPtr = std::shared_ptr<Client>;

std::string CreateTmpKey(const std::string& key, std::string prefix);

}  // namespace redis
}  // namespace storages
