#pragma once

/// @file storages/redis/client.hpp
/// @brief @copybrief storages::redis::Client

#include <chrono>
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

class ScanOptionsBase {
 public:
  ScanOptionsBase() = default;
  ScanOptionsBase(ScanOptionsBase& other) = default;
  ScanOptionsBase(const ScanOptionsBase& other) = default;

  ScanOptionsBase(ScanOptionsBase&& other) = default;

  template <typename... Args>
  ScanOptionsBase(Args&&... args) {
    (Apply(std::forward<Args>(args)), ...);
  }

  class Match {
   public:
    explicit Match(std::string value) : value_(std::move(value)) {}

    const std::string& Get() const& { return value_; }

    std::string Get() && { return std::move(value_); }

   private:
    std::string value_;
  };

  class Count {
   public:
    explicit Count(size_t value) : value_(value) {}

    size_t Get() const { return value_; }

   private:
    size_t value_;
  };

  boost::optional<Match> ExtractMatch() { return std::move(pattern_); }

  boost::optional<Count> ExtractCount() { return std::move(count_); }

 private:
  void Apply(Match pattern) {
    if (pattern_)
      throw ::redis::InvalidArgumentException("duplicate Match parameter");
    pattern_ = std::move(pattern);
  }

  void Apply(Count count) {
    if (count_)
      throw ::redis::InvalidArgumentException("duplicate Count parameter");
    count_ = count;
  }

  boost::optional<Match> pattern_;
  boost::optional<Count> count_;
};

// strong typedef
template <ScanTag scan_tag>
class ScanOptionsTmpl : public ScanOptionsBase {
  using ScanOptionsBase::ScanOptionsBase;
};

using ScanOptions = ScanOptionsTmpl<ScanTag::kScan>;
using SscanOptions = ScanOptionsTmpl<ScanTag::kSscan>;
using HscanOptions = ScanOptionsTmpl<ScanTag::kHscan>;
using ZscanOptions = ScanOptionsTmpl<ScanTag::kZscan>;

void PutArg(::redis::CmdArgs::CmdArgsArray& args_,
            boost::optional<ScanOptionsBase::Match> arg);
void PutArg(::redis::CmdArgs::CmdArgsArray& args_,
            boost::optional<ScanOptionsBase::Count> arg);

enum class PubShard {
  kZeroShard,
  kRoundRobin,
};

using RetryNilFromMaster = ::redis::RetryNilFromMaster;

static const RetryNilFromMaster kRetryNilFromMaster{};

/// @class Client
class Client {
 public:
  virtual ~Client() = default;

  virtual size_t ShardsCount() const = 0;

  virtual size_t ShardByKey(const std::string& key) const = 0;

  virtual const std::string& GetAnyKeyForShard(size_t shard_idx) const = 0;

  // redis commands:

  virtual RequestAppend Append(std::string key, std::string value,
                               const CommandControl& command_control) = 0;

  virtual RequestDbsize Dbsize(size_t shard,
                               const CommandControl& command_control) = 0;

  virtual RequestDel Del(std::string key,
                         const CommandControl& command_control) = 0;

  virtual RequestDel Del(std::vector<std::string> keys,
                         const CommandControl& command_control) = 0;

  virtual RequestEval Eval(std::string script, std::vector<std::string> keys,
                           std::vector<std::string> args,
                           const CommandControl& command_control) = 0;

  virtual RequestExists Exists(std::string key,
                               const CommandControl& command_control) = 0;

  virtual RequestExists Exists(std::vector<std::string> keys,
                               const CommandControl& command_control) = 0;

  virtual RequestExpire Expire(std::string key, std::chrono::seconds ttl,
                               const CommandControl& command_control) = 0;

  virtual RequestGet Get(std::string key,
                         const CommandControl& command_control) = 0;

  virtual RequestGet Get(std::string key, RetryNilFromMaster,
                         const CommandControl& command_control) = 0;

  virtual RequestHdel Hdel(std::string key, std::string field,
                           const CommandControl& command_control) = 0;

  virtual RequestHdel Hdel(std::string key, std::vector<std::string> fields,
                           const CommandControl& command_control) = 0;

  virtual RequestHexists Hexists(std::string key, std::string field,
                                 const CommandControl& command_control) = 0;

  virtual RequestHget Hget(std::string key, std::string field,
                           const CommandControl& command_control) = 0;

  virtual RequestHget Hget(std::string key, std::string field,
                           RetryNilFromMaster,
                           const CommandControl& command_control) = 0;

  // use Hscan in case of a big hash
  virtual RequestHgetall Hgetall(std::string key,
                                 const CommandControl& command_control) = 0;

  virtual RequestHincrby Hincrby(std::string key, std::string field,
                                 int64_t increment,
                                 const CommandControl& command_control) = 0;

  virtual RequestHincrbyfloat Hincrbyfloat(
      std::string key, std::string field, double increment,
      const CommandControl& command_control) = 0;

  // use Hscan in case of a big hash
  virtual RequestHkeys Hkeys(std::string key,
                             const CommandControl& command_control) = 0;

  virtual RequestHlen Hlen(std::string key,
                           const CommandControl& command_control) = 0;

  virtual RequestHmget Hmget(std::string key, std::vector<std::string> fields,
                             const CommandControl& command_control) = 0;

  virtual RequestHmset Hmset(
      std::string key,
      std::vector<std::pair<std::string, std::string>> field_values,
      const CommandControl& command_control) = 0;

  virtual RequestHscan Hscan(std::string key,
                             const CommandControl& command_control) = 0;

  virtual RequestHscan Hscan(std::string key, HscanOptions options,
                             const CommandControl& command_control) = 0;

  virtual RequestHset Hset(std::string key, std::string field,
                           std::string value,
                           const CommandControl& command_control) = 0;

  virtual RequestHsetnx Hsetnx(std::string key, std::string field,
                               std::string value,
                               const CommandControl& command_control) = 0;

  // use Hscan in case of a big hash
  virtual RequestHvals Hvals(std::string key,
                             const CommandControl& command_control) = 0;

  virtual RequestIncr Incr(std::string key,
                           const CommandControl& command_control) = 0;

  [[deprecated("use Scan")]] virtual RequestKeys Keys(
      std::string keys_pattern, size_t shard,
      const CommandControl& command_control) = 0;

  virtual RequestLindex Lindex(std::string key, int64_t index,
                               const CommandControl& command_control) = 0;

  virtual RequestLlen Llen(std::string key,
                           const CommandControl& command_control) = 0;

  virtual RequestLpop Lpop(std::string key,
                           const CommandControl& command_control) = 0;

  virtual RequestLpush Lpush(std::string key, std::string value,
                             const CommandControl& command_control) = 0;

  virtual RequestLpush Lpush(std::string key, std::vector<std::string> values,
                             const CommandControl& command_control) = 0;

  virtual RequestLrange Lrange(std::string key, int64_t start, int64_t stop,
                               const CommandControl& command_control) = 0;

  virtual RequestLtrim Ltrim(std::string key, int64_t start, int64_t stop,
                             const CommandControl& command_control) = 0;

  virtual RequestMget Mget(std::vector<std::string> keys,
                           const CommandControl& command_control) = 0;

  virtual RequestPersist Persist(std::string key,
                                 const CommandControl& command_control) = 0;

  virtual RequestPexpire Pexpire(std::string key, std::chrono::milliseconds ttl,
                                 const CommandControl& command_control) = 0;

  virtual RequestPing Ping(size_t shard,
                           const CommandControl& command_control) = 0;

  virtual RequestPingMessage Ping(size_t shard, std::string message,
                                  const CommandControl& command_control) = 0;

  virtual void Publish(std::string channel, std::string message,
                       const CommandControl& command_control) = 0;

  virtual void Publish(std::string channel, std::string message,
                       const CommandControl& command_control,
                       PubShard policy) = 0;

  virtual RequestRename Rename(std::string key, std::string new_key,
                               const CommandControl& command_control) = 0;

  virtual RequestRpop Rpop(std::string key,
                           const CommandControl& command_control) = 0;

  virtual RequestRpush Rpush(std::string key, std::string value,
                             const CommandControl& command_control) = 0;

  virtual RequestRpush Rpush(std::string key, std::vector<std::string> values,
                             const CommandControl& command_control) = 0;

  virtual RequestSadd Sadd(std::string key, std::string member,
                           const CommandControl& command_control) = 0;

  virtual RequestSadd Sadd(std::string key, std::vector<std::string> members,
                           const CommandControl& command_control) = 0;

  virtual RequestScan Scan(size_t shard,
                           const CommandControl& command_control) = 0;

  virtual RequestScan Scan(size_t shard, ScanOptions options,
                           const CommandControl& command_control) = 0;

  virtual RequestScard Scard(std::string key,
                             const CommandControl& command_control) = 0;

  virtual RequestSet Set(std::string key, std::string value,
                         const CommandControl& command_control) = 0;

  virtual RequestSet Set(std::string key, std::string value,
                         std::chrono::milliseconds ttl,
                         const CommandControl& command_control) = 0;

  virtual RequestSetIfExist SetIfExist(
      std::string key, std::string value,
      const CommandControl& command_control) = 0;

  virtual RequestSetIfExist SetIfExist(
      std::string key, std::string value, std::chrono::milliseconds ttl,
      const CommandControl& command_control) = 0;

  virtual RequestSetIfNotExist SetIfNotExist(
      std::string key, std::string value,
      const CommandControl& command_control) = 0;

  virtual RequestSetIfNotExist SetIfNotExist(
      std::string key, std::string value, std::chrono::milliseconds ttl,
      const CommandControl& command_control) = 0;

  virtual RequestSetex Setex(std::string key, std::chrono::seconds seconds,
                             std::string value,
                             const CommandControl& command_control) = 0;

  virtual RequestSismember Sismember(std::string key, std::string member,
                                     const CommandControl& command_control) = 0;

  // use Sscan in case of a big set
  virtual RequestSmembers Smembers(std::string key,
                                   const CommandControl& command_control) = 0;

  virtual RequestSrandmember Srandmember(
      std::string key, const CommandControl& command_control) = 0;

  virtual RequestSrandmembers Srandmembers(
      std::string key, int64_t count,
      const CommandControl& command_control) = 0;

  virtual RequestSrem Srem(std::string key, std::string member,
                           const CommandControl& command_control) = 0;

  virtual RequestSrem Srem(std::string key, std::vector<std::string> members,
                           const CommandControl& command_control) = 0;

  virtual RequestSscan Sscan(std::string key,
                             const CommandControl& command_control) = 0;

  virtual RequestSscan Sscan(std::string key, SscanOptions options,
                             const CommandControl& command_control) = 0;

  virtual RequestStrlen Strlen(std::string key,
                               const CommandControl& command_control) = 0;

  virtual RequestTtl Ttl(std::string key,
                         const CommandControl& command_control) = 0;

  virtual RequestType Type(std::string key,
                           const CommandControl& command_control) = 0;

  virtual RequestZadd Zadd(std::string key, double score, std::string member,
                           const CommandControl& command_control) = 0;

  virtual RequestZadd Zadd(std::string key, double score, std::string member,
                           const ZaddOptions& options,
                           const CommandControl& command_control) = 0;

  virtual RequestZaddIncr ZaddIncr(std::string key, double score,
                                   std::string member,
                                   const CommandControl& command_control) = 0;

  virtual RequestZaddIncrExisting ZaddIncrExisting(
      std::string key, double score, std::string member,
      const CommandControl& command_control) = 0;

  virtual RequestZcard Zcard(std::string key,
                             const CommandControl& command_control) = 0;

  virtual RequestZrangebyscore Zrangebyscore(
      std::string key, double min, double max,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscore Zrangebyscore(
      std::string key, std::string min, std::string max,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscore Zrangebyscore(
      std::string key, double min, double max,
      const RangeOptions& range_options,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscore Zrangebyscore(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, double min, double max,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, double min, double max,
      const RangeOptions& range_options,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options,
      const CommandControl& command_control) = 0;

  virtual RequestZrem Zrem(std::string key, std::string member,
                           const CommandControl& command_control) = 0;

  virtual RequestZrem Zrem(std::string key, std::vector<std::string> members,
                           const CommandControl& command_control) = 0;

  virtual RequestZscan Zscan(std::string key,
                             const CommandControl& command_control) = 0;

  virtual RequestZscan Zscan(std::string key, ZscanOptions options,
                             const CommandControl& command_control) = 0;

  virtual RequestZscore Zscore(std::string key, std::string member,
                               const CommandControl& command_control) = 0;

  virtual RequestZscore Zscore(std::string key, std::string member,
                               RetryNilFromMaster,
                               const CommandControl& command_control) = 0;
};

using ClientPtr = std::shared_ptr<Client>;

std::string CreateTmpKey(const std::string& key, std::string prefix);

}  // namespace redis
}  // namespace storages
