#include <userver/storages/redis/client.hpp>

#include <storages/redis/impl/sentinel.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

std::string CreateTmpKey(const std::string& key, std::string prefix) {
  return USERVER_NAMESPACE::redis::Sentinel::CreateTmpKey(key,
                                                          std::move(prefix));
}

void Client::CheckShardIdx(size_t shard_idx) const {
  USERVER_NAMESPACE::redis::Sentinel::CheckShardIdx(shard_idx, ShardsCount());
}

RequestGet Client::Get(std::string key, RetryNilFromMaster,
                       const CommandControl& command_control) {
  return Get(std::move(key), command_control.MergeWith(kRetryNilFromMaster));
}

RequestHget Client::Hget(std::string key, std::string field, RetryNilFromMaster,
                         const CommandControl& command_control) {
  return Hget(std::move(key), std::move(field),
              command_control.MergeWith(kRetryNilFromMaster));
}

RequestZscore Client::Zscore(std::string key, std::string member,
                             RetryNilFromMaster,
                             const CommandControl& command_control) {
  return Zscore(std::move(key), std::move(member),
                command_control.MergeWith(kRetryNilFromMaster));
}

void Client::Publish(std::string channel, std::string message,
                     const CommandControl& command_control) {
  return Publish(std::move(channel), std::move(message), command_control,
                 PubShard::kZeroShard);
}

ScanRequest<ScanTag::kScan> Client::Scan(
    size_t shard, const CommandControl& command_control) {
  return Scan(shard, {}, command_control);
}

ScanRequest<ScanTag::kHscan> Client::Hscan(
    std::string key, const CommandControl& command_control) {
  return Hscan(std::move(key), {}, command_control);
}

ScanRequest<ScanTag::kSscan> Client::Sscan(
    std::string key, const CommandControl& command_control) {
  return Sscan(std::move(key), {}, command_control);
}

ScanRequest<ScanTag::kZscan> Client::Zscan(
    std::string key, const CommandControl& command_control) {
  return Zscan(std::move(key), {}, command_control);
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
