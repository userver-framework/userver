#include <userver/storages/redis/impl/base.hpp>

#include <mutex>
#include <sstream>
#include <unordered_map>

#include <userver/logging/log_helper.hpp>
#include <userver/utils/text.hpp>

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

void PutArg(CmdArgs::CmdArgsArray& args_, const char* arg) {
  args_.emplace_back(arg);
}

void PutArg(CmdArgs::CmdArgsArray& args_, const std::string& arg) {
  args_.emplace_back(arg);
}

void PutArg(CmdArgs::CmdArgsArray& args_, std::string&& arg) {
  args_.emplace_back(std::move(arg));
}

void PutArg(CmdArgs::CmdArgsArray& args_, const std::vector<std::string>& arg) {
  for (const auto& str : arg) args_.emplace_back(str);
}

void PutArg(CmdArgs::CmdArgsArray& args_,
            const std::vector<std::pair<std::string, std::string>>& arg) {
  for (const auto& pair : arg) {
    args_.emplace_back(pair.first);
    args_.emplace_back(pair.second);
  }
}

void PutArg(CmdArgs::CmdArgsArray& args_,
            const std::vector<std::pair<double, std::string>>& arg) {
  for (const auto& pair : arg) {
    args_.emplace_back(std::to_string(pair.first));
    args_.emplace_back(pair.second);
  }
}

logging::LogHelper& operator<<(logging::LogHelper& os, const CmdArgs& v) {
  constexpr std::size_t kArgSizeLimit = 1024;

  if (v.args.size() > 1) os << "[";
  bool first = true;
  for (const auto& arg_array : v.args) {
    if (first)
      first = false;
    else
      os << ", ";

    if (os.IsLimitReached()) {
      os << "...";
      break;
    }

    os << "\"";
    bool first_arg = true;
    for (const auto& arg : arg_array) {
      if (first_arg)
        first_arg = false;
      else {
        os << " ";
        if (os.IsLimitReached()) {
          os << "...";
          break;
        }
      }

      if (utils::text::IsUtf8(arg)) {
        if (arg.size() <= kArgSizeLimit) {
          os << arg;
        } else {
          std::string_view view{arg};
          view = view.substr(0, kArgSizeLimit);
          utils::text::utf8::TrimViewTruncatedEnding(view);
          os << view << "<...>";
        }
      } else {
        os << "<bin:" << arg.size() << ">";
      }
    }
    os << "\"";
  }
  if (v.args.size() > 1) os << "]";
  return os;
}

CommandControl::CommandControl(std::chrono::milliseconds timeout_single,
                               std::chrono::milliseconds timeout_all,
                               size_t max_retries,
                               CommandControl::Strategy strategy,
                               int best_dc_count,
                               std::chrono::milliseconds max_ping_latency)
    : timeout_single(timeout_single),
      timeout_all(timeout_all),
      max_retries(max_retries),
      strategy(strategy),
      best_dc_count(best_dc_count),
      max_ping_latency(max_ping_latency) {}

CommandControl CommandControl::MergeWith(const CommandControl& b) const {
  CommandControl res(*this);
  if (b.timeout_single > std::chrono::milliseconds(0))
    res.timeout_single = b.timeout_single;
  if (b.timeout_all > std::chrono::milliseconds(0))
    res.timeout_all = b.timeout_all;
  if (b.max_retries > 0) res.max_retries = b.max_retries;
  if (b.strategy != Strategy::kDefault) res.strategy = b.strategy;
  if (b.best_dc_count > 0) res.best_dc_count = b.best_dc_count;
  if (b.max_ping_latency > std::chrono::milliseconds(0))
    res.max_ping_latency = b.max_ping_latency;
  if (b.force_request_to_master)
    res.force_request_to_master = b.force_request_to_master;
  if (b.allow_reads_from_master)
    res.allow_reads_from_master = b.allow_reads_from_master;
  if (!b.force_server_id.IsAny()) res.force_server_id = b.force_server_id;
  if (b.force_retries_to_master_on_nil_reply)
    res.force_retries_to_master_on_nil_reply =
        b.force_retries_to_master_on_nil_reply;
  if (b.force_shard_idx) res.force_shard_idx = b.force_shard_idx;
  return res;
}

CommandControl CommandControl::MergeWith(
    const testsuite::RedisControl& t) const {
  CommandControl res(*this);
  res.timeout_single = std::max(t.min_timeout_single, res.timeout_single);
  res.timeout_all = std::max(t.min_timeout_all, res.timeout_all);
  return res;
}

std::string CommandControl::ToString() const {
  std::stringstream ss;
  ss << "timeouts: " << timeout_single.count() << ' ' << timeout_all.count()
     << ',';
  ss << " strategy: " << static_cast<int>(strategy) << ',';
  ss << " best_dc_count: " << best_dc_count << ',';
  ss << " retries: " << max_retries << ',';
  if (!force_server_id.IsAny())
    ss << " force_server_id: " << force_server_id.GetId() << ',';
  if (force_request_to_master) ss << " force_request_to_master: true,";
  if (force_shard_idx) ss << " force_shard_idx: " << *force_shard_idx << ',';
  ss << " max ping: " << max_ping_latency.count();
  return ss.str();
}

CommandControl CommandControl::MergeWith(RetryNilFromMaster) const {
  CommandControl res(*this);
  res.force_retries_to_master_on_nil_reply = true;
  return res;
}

CommandControl::Strategy StrategyFromString(const std::string& strategy) {
  if (strategy == "every_dc") {
    return redis::CommandControl::Strategy::kEveryDc;
  } else if (strategy == "default") {
    return redis::CommandControl::Strategy::kDefault;
  } else if (strategy == "local_dc_conductor") {
    return redis::CommandControl::Strategy::kLocalDcConductor;
  } else if (strategy == "nearest_server_ping") {
    return redis::CommandControl::Strategy::kNearestServerPing;
  } else {
    throw std::runtime_error(
        "Unknown strategy for redis::CommandControl::Strategy (" + strategy +
        ")");
  }
}

}  // namespace redis

USERVER_NAMESPACE_END
