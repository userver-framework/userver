#pragma once

/// @file storages/redis/client_impl.hpp
/// @brief @copybrief storages::redis::ClientImpl

#include <memory>
#include <string>

#include <redis/base.hpp>
#include <redis/command_options.hpp>

#include <storages/redis/client.hpp>

namespace redis {
class Sentinel;
}  // namespace redis

namespace storages {
namespace redis {

/// @class ClientImpl
class ClientImpl final : public Client {
 public:
  explicit ClientImpl(std::shared_ptr<::redis::Sentinel> sentinel);

  size_t ShardsCount() const override;

  size_t ShardByKey(const std::string& key) const override;

  const std::string& GetAnyKeyForShard(size_t shard_idx) const override;

  // redis commands:

  RequestAppend Append(const std::string& key, std::string value,
                       const CommandControl& command_control) override;

  RequestDel Del(const std::string& key,
                 const CommandControl& command_control) override;

  RequestGet Get(const std::string& key,
                 const CommandControl& command_control) override;

  RequestHdel Hdel(const std::string& key, const std::string& field,
                   const CommandControl& command_control) override;

  RequestHget Hget(const std::string& key, const std::string& field,
                   const CommandControl& command_control) override;

  RequestHgetall Hgetall(const std::string& key,
                         const CommandControl& command_control) override;

  RequestHmset Hmset(
      const std::string& key,
      const std::vector<std::pair<std::string, std::string>>& field_val,
      const CommandControl& command_control) override;

  RequestHset Hset(const std::string& key, const std::string& field,
                   const std::string& value,
                   const CommandControl& command_control) override;

  RequestKeys Keys(std::string keys_pattern, size_t shard,
                   const CommandControl& command_control) override;

  RequestMget Mget(std::vector<std::string> keys,
                   const CommandControl& command_control) override;

  RequestPing Ping(size_t shard,
                   const CommandControl& command_control) override;

  RequestPingMessage Ping(size_t shard, std::string message,
                          const CommandControl& command_control) override;

  RequestPublish Publish(const std::string& channel, std::string message,
                         const CommandControl& command_control) override;

  RequestPublish Publish(const std::string& channel, std::string message,
                         const CommandControl& command_control,
                         PubShard policy) override;

  RequestSet Set(const std::string& key, std::string value,
                 const CommandControl& command_control) override;

  RequestSetex Setex(const std::string& key, int64_t seconds, std::string value,
                     const CommandControl& command_control) override;

  RequestStrlen Strlen(const std::string& key,
                       const CommandControl& command_control) override;

  RequestTtl Ttl(const std::string& key,
                 const CommandControl& command_control) override;

  RequestType Type(const std::string& key,
                   const CommandControl& command_control) override;

  RequestZadd Zadd(const std::string& key, double score,
                   const std::string& member,
                   const CommandControl& command_control) override;

  RequestZadd Zadd(const std::string& key, double score,
                   const std::string& member, const ZaddOptions& options,
                   const CommandControl& command_control) override;

  RequestZrangebyscore Zrangebyscore(
      const std::string& key, double min, double max,
      const RangeOptions& range_options,
      const CommandControl& command_control) override;

  RequestZrangebyscore Zrangebyscore(
      const std::string& key, const std::string& min, const std::string& max,
      const RangeOptions& range_options,
      const CommandControl& command_control) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      const std::string& key, double min, double max,
      const RangeOptions& range_options,
      const CommandControl& command_control) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      const std::string& key, const std::string& min, const std::string& max,
      const RangeOptions& range_options,
      const CommandControl& command_control) override;

  RequestZrem Zrem(const std::string& key, const std::string& member,
                   const CommandControl& command_control) override;

  RequestZscore Zscore(const std::string& key, const std::string& member,
                       const CommandControl& command_control) override;

 private:
  using CmdArgs = ::redis::CmdArgs;

  ::redis::Request MakeRequest(CmdArgs&& args, const std::string& key,
                               bool master,
                               const CommandControl& command_control);

  ::redis::Request MakeRequest(CmdArgs&& args, size_t shard, bool master,
                               const CommandControl& command_control);

  CommandControl GetCommandControl(const CommandControl& cc) const;

  size_t GetPublishShard(PubShard policy);

  std::shared_ptr<::redis::Sentinel> redis_client_;
  std::atomic<int> publish_shard_{0};
};

}  // namespace redis
}  // namespace storages
