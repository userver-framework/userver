#include <userver/storages/redis/command_control.hpp>

#include <mutex>
#include <sstream>
#include <unordered_map>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <userver/testsuite/redis_control.hpp>
#include <userver/utils/optionals.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {
namespace {

class ServerIdDescriptionMap {
 public:
  ServerIdDescriptionMap() {
    SetDescription(ServerId::Invalid().GetId(), "invalid_server_id");
  }

  void SetDescription(size_t server_id, std::string description) {
    std::lock_guard<std::mutex> lock(mutex_);
    descriptions_.emplace(std::piecewise_construct, std::tie(server_id),
                          std::tie(description));
  }

  void RemoveDescription(size_t server_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    descriptions_.erase(server_id);
  }

  std::string GetDescription(size_t server_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = descriptions_.find(server_id);
    if (it != descriptions_.end()) return it->second;
    return {};
  }

 private:
  mutable std::mutex mutex_;
  std::unordered_map<size_t, std::string> descriptions_;
};

ServerIdDescriptionMap& GetServerIdDescriptionMap() {
  static ServerIdDescriptionMap description_map;
  return description_map;
}

std::optional<int> ToIntOpt(
    const std::optional<CommandControl::Strategy>& strategy) {
  if (!strategy) return std::nullopt;
  return static_cast<int>(*strategy);
}

std::optional<int64_t> ToIntOpt(const std::optional<ServerId>& server_id) {
  if (!server_id || server_id->IsAny()) return std::nullopt;
  return server_id->GetId();
}

}  // namespace

void ServerId::SetDescription(std::string description) const {
  if (IsAny()) return;
  GetServerIdDescriptionMap().SetDescription(id_, std::move(description));
}

void ServerId::RemoveDescription() const {
  if (IsAny()) return;
  GetServerIdDescriptionMap().RemoveDescription(id_);
}

std::string ServerId::GetDescription() const {
  return GetServerIdDescriptionMap().GetDescription(id_);
}

std::atomic<int64_t> ServerId::next_id_{0};
ServerId ServerId::invalid_ = ServerId::Generate();

CommandControl::CommandControl(
    const std::optional<std::chrono::milliseconds>& timeout_single,
    const std::optional<std::chrono::milliseconds>& timeout_all,
    const std::optional<size_t>& max_retries)
    : timeout_single(timeout_single),
      timeout_all(timeout_all),
      max_retries(max_retries) {}

CommandControl CommandControl::MergeWith(const CommandControl& b) const {
  CommandControl res(*this);
  if (b.timeout_single.has_value() &&
      *b.timeout_single > std::chrono::milliseconds::zero()) {
    res.timeout_single = b.timeout_single;
  }
  if (b.timeout_all.has_value() &&
      *b.timeout_all > std::chrono::milliseconds::zero()) {
    res.timeout_all = b.timeout_all;
  }
  if (b.max_retries.has_value()) {
    res.max_retries = b.max_retries;
  }
  if (b.strategy.has_value()) {
    res.strategy = b.strategy;
  }
  if (b.best_dc_count.has_value()) {
    res.best_dc_count = b.best_dc_count;
  }
  if (b.force_request_to_master.has_value()) {
    res.force_request_to_master = b.force_request_to_master;
  }
  if (b.max_ping_latency.has_value()) {
    res.max_ping_latency = b.max_ping_latency;
  }
  if (b.allow_reads_from_master.has_value()) {
    res.allow_reads_from_master = b.allow_reads_from_master;
  }
  if (b.account_in_statistics.has_value()) {
    res.account_in_statistics = b.account_in_statistics;
  }
  if (b.force_shard_idx.has_value()) {
    res.force_shard_idx = b.force_shard_idx;
  }
  if (b.chunk_size.has_value()) {
    res.chunk_size = b.chunk_size;
  }
  if (b.force_server_id.has_value()) {
    res.force_server_id = b.force_server_id;
  }
  return (b.force_retries_to_master_on_nil_reply
              ? res.MergeWith(RetryNilFromMaster{})
              : res);
}

CommandControl CommandControl::MergeWith(
    const testsuite::RedisControl& t) const {
  CommandControl res(*this);
  if (t.min_timeout_single > std::chrono::milliseconds::zero()) {
    res.timeout_single =
        (res.timeout_single.has_value()
             ? std::max(*res.timeout_single, t.min_timeout_single)
             : t.min_timeout_single);
  }
  if (t.min_timeout_all > std::chrono::milliseconds::zero()) {
    res.timeout_all = (res.timeout_all.has_value()
                           ? std::max(*res.timeout_all, t.min_timeout_all)
                           : t.min_timeout_all);
  }
  return res;
}

CommandControl CommandControl::MergeWith(RetryNilFromMaster) const {
  CommandControl res(*this);
  res.force_retries_to_master_on_nil_reply = true;
  return res;
}

std::string CommandControl::ToString() const {
  std::ostringstream ss;
  ss << "timeouts: " << utils::ToString(timeout_single) << ' '
     << utils::ToString(timeout_all) << ',';
  ss << " strategy: " << utils::ToString(ToIntOpt(strategy)) << ',';
  ss << " best_dc_count: " << utils::ToString(best_dc_count) << ',';
  ss << " retries: " << utils::ToString(max_retries) << ',';
  ss << " force_server_id: " << utils::ToString(ToIntOpt(force_server_id))
     << ',';
  ss << " max ping: " << utils::ToString(max_ping_latency);
  return ss.str();
}

CommandControl::Strategy StrategyFromString(std::string_view strategy) {
  using Strategy = CommandControl::Strategy;
  constexpr utils::TrivialBiMap kToStrategy = [](auto selector) {
    return selector()
        .Case("every_dc", Strategy::kEveryDc)
        .Case("default", Strategy::kDefault)
        .Case("local_dc_conductor", Strategy::kLocalDcConductor)
        .Case("nearest_server_ping", Strategy::kNearestServerPing);
  };

  auto result = kToStrategy.TryFind(strategy);
  if (!result) {
    throw std::runtime_error(
        fmt::format("redis::CommandControl::Strategy should be one of [{}], "
                    "but '{}' was given",
                    kToStrategy.DescribeFirst(), strategy));
  }

  return *result;
}

}  // namespace redis

USERVER_NAMESPACE_END
