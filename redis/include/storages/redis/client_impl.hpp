#pragma once

/// @file storages/redis/client_impl.hpp
/// @brief @copybrief storages::redis::ClientImpl

#include <chrono>
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

  RequestAppend Append(std::string key, std::string value,
                       const CommandControl& command_control) override;

  RequestDbsize Dbsize(size_t shard,
                       const CommandControl& command_control) override;

  RequestDel Del(std::string key,
                 const CommandControl& command_control) override;

  RequestDel Del(std::vector<std::string> keys,
                 const CommandControl& command_control) override;

  RequestEval Eval(std::string script, std::vector<std::string> keys,
                   std::vector<std::string> args,
                   const CommandControl& command_control) override;

  RequestExists Exists(std::string key,
                       const CommandControl& command_control) override;

  RequestExists Exists(std::vector<std::string> keys,
                       const CommandControl& command_control) override;

  RequestGet Get(std::string key,
                 const CommandControl& command_control) override;

  RequestHdel Hdel(std::string key, std::string field,
                   const CommandControl& command_control) override;

  RequestHdel Hdel(std::string key, std::vector<std::string> fields,
                   const CommandControl& command_control) override;

  RequestHget Hget(std::string key, std::string field,
                   const CommandControl& command_control) override;

  RequestHgetall Hgetall(std::string key,
                         const CommandControl& command_control) override;

  RequestHmset Hmset(
      std::string key,
      std::vector<std::pair<std::string, std::string>> field_values,
      const CommandControl& command_control) override;

  RequestHset Hset(std::string key, std::string field, std::string value,
                   const CommandControl& command_control) override;

  RequestKeys Keys(std::string keys_pattern, size_t shard,
                   const CommandControl& command_control) override;

  RequestMget Mget(std::vector<std::string> keys,
                   const CommandControl& command_control) override;

  RequestPing Ping(size_t shard,
                   const CommandControl& command_control) override;

  RequestPingMessage Ping(size_t shard, std::string message,
                          const CommandControl& command_control) override;

  void Publish(std::string channel, std::string message,
               const CommandControl& command_control) override;

  void Publish(std::string channel, std::string message,
               const CommandControl& command_control, PubShard policy) override;

  RequestSet Set(std::string key, std::string value,
                 const CommandControl& command_control) override;

  RequestSet Set(std::string key, std::string value,
                 std::chrono::milliseconds ttl,
                 const CommandControl& command_control) override;

  RequestSetIfExist SetIfExist(std::string key, std::string value,
                               const CommandControl& command_control) override;

  RequestSetIfExist SetIfExist(std::string key, std::string value,
                               std::chrono::milliseconds ttl,
                               const CommandControl& command_control) override;

  RequestSetIfNotExist SetIfNotExist(
      std::string key, std::string value,
      const CommandControl& command_control) override;

  RequestSetIfNotExist SetIfNotExist(
      std::string key, std::string value, std::chrono::milliseconds ttl,
      const CommandControl& command_control) override;

  RequestSetex Setex(std::string key, std::chrono::seconds seconds,
                     std::string value,
                     const CommandControl& command_control) override;

  RequestSismember Sismember(std::string key, std::string member,
                             const CommandControl& command_control) override;

  RequestStrlen Strlen(std::string key,
                       const CommandControl& command_control) override;

  RequestTtl Ttl(std::string key,
                 const CommandControl& command_control) override;

  RequestType Type(std::string key,
                   const CommandControl& command_control) override;

  RequestZadd Zadd(std::string key, double score, std::string member,
                   const CommandControl& command_control) override;

  RequestZadd Zadd(std::string key, double score, std::string member,
                   const ZaddOptions& options,
                   const CommandControl& command_control) override;

  RequestZaddIncr ZaddIncr(std::string key, double score, std::string member,
                           const CommandControl& command_control) override;

  RequestZaddIncrExisting ZaddIncrExisting(
      std::string key, double score, std::string member,
      const CommandControl& command_control) override;

  RequestZcard Zcard(std::string key,
                     const CommandControl& command_control) override;

  RequestZrangebyscore Zrangebyscore(
      std::string key, double min, double max,
      const CommandControl& command_control) override;

  RequestZrangebyscore Zrangebyscore(
      std::string key, std::string min, std::string max,
      const CommandControl& command_control) override;

  RequestZrangebyscore Zrangebyscore(
      std::string key, double min, double max,
      const RangeOptions& range_options,
      const CommandControl& command_control) override;

  RequestZrangebyscore Zrangebyscore(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options,
      const CommandControl& command_control) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, double min, double max,
      const CommandControl& command_control) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max,
      const CommandControl& command_control) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, double min, double max,
      const RangeOptions& range_options,
      const CommandControl& command_control) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options,
      const CommandControl& command_control) override;

  RequestZrem Zrem(std::string key, std::string member,
                   const CommandControl& command_control) override;

  RequestZrem Zrem(std::string key, std::vector<std::string> members,
                   const CommandControl& command_control) override;

  RequestZscore Zscore(std::string key, std::string member,
                       const CommandControl& command_control) override;

 private:
  using CmdArgs = ::redis::CmdArgs;

  ::redis::Request MakeRequest(CmdArgs&& args, size_t shard, bool master,
                               const CommandControl& command_control);

  CommandControl GetCommandControl(const CommandControl& cc) const;

  size_t GetPublishShard(PubShard policy);

  std::shared_ptr<::redis::Sentinel> redis_client_;
  std::atomic<int> publish_shard_{0};
};

}  // namespace redis
}  // namespace storages
