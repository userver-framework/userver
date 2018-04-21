#include "sentinel_impl.hpp"

#include <cassert>

#include <boost/crc.hpp>

#include <logging/logger.hpp>

#include "reply.hpp"

namespace storages {
namespace redis {
namespace {

bool ParseSentinelResponse(
    const ReplyData& reply, int status, bool allow_empty,
    std::vector<std::map<std::string, std::string>>& res) {
  res.clear();

  if (!reply || status != REDIS_OK ||
      reply.GetType() != ReplyData::Type::kArray ||
      (!allow_empty && reply.GetArray().empty())) {
    std::stringstream ss;
    ss << "can't parse sentinel response. status=" << status;
    if (reply) {
      ss << " type=" << static_cast<int>(reply.GetType());
      ss << " msg=" << reply.ToString();
    }
    LOG_WARNING() << ss.str();
    return false;
  }

  for (const ReplyData& reply_in : reply.GetArray()) {
    if (reply_in && reply_in.GetType() == ReplyData::Type::kArray &&
        !reply_in.GetArray().empty()) {
      const ReplyData::ReplyArray& array = reply_in.GetArray();
      res.push_back(std::map<std::string, std::string>());
      auto& properties = res.back();
      for (size_t k = 0; k < array.size() - 1; k += 2)
        properties[array[k].ToString()] = array[k + 1].ToString();
    }
  }

  return true;
}

std::set<std::string> SentinelParseFlags(const std::string& flags) {
  std::set<std::string> res;
  size_t l = 0, r;

  do {
    r = flags.find(',', l);
    std::string flag = flags.substr(l, r - l);
    if (!flag.empty()) res.insert(flag);
    l = r + 1;
  } while (r != std::string::npos);

  return res;
}

bool SentinelNodeIsActive(std::map<std::string, std::string>& properties) {
  auto flags = SentinelParseFlags(properties["flags"]);
  bool master = flags.find("master") != flags.end();
  bool slave = flags.find("slave") != flags.end();

  if (!master && !slave) return false;

  if (flags.find("s_down") != flags.end() ||
      flags.find("o_down") != flags.end() ||
      flags.find("disconnected") != flags.end())
    return false;

  if (slave && properties["master-link-status"] != "ok") return false;

  if (slave && properties["slave-priority"] == "0") return false;

  return true;
}

}  // namespace

SentinelImpl::SentinelImpl(const engine::ev::ThreadControl& thread_control,
                           engine::ev::ThreadPool& thread_pool,
                           const std::vector<std::string>& shards,
                           const std::vector<ConnectionInfo>& conns,
                           std::string password,
                           ReadyChangeCallback ready_callback,
                           std::unique_ptr<KeyShard>&& key_shard,
                           bool track_masters, bool track_slaves)
    : engine::ev::ThreadControl(thread_control),
      thread_pool_(thread_pool),
      loop_(GetEvLoop()),
      password_(std::move(password)),
      track_masters_(track_masters),
      track_slaves_(track_slaves),
      key_shard_(std::move(key_shard)) {
  assert(loop_ != nullptr);
  auto init_shards = [&shards, &ready_callback](
                         std::vector<std::shared_ptr<Shard>>& shard_objects,
                         bool master) {
    size_t i = 0;
    for (const auto& shard : shards) {
      auto object = std::make_shared<Shard>();
      object->name = shard;
      object->read_only = !master;
      object->ready_change_callback = [i, shard, ready_callback,
                                       master](bool ready) {
        if (ready_callback) ready_callback(i, shard, master, ready);
      };
      shard_objects.emplace_back(std::move(object));
      ++i;
    }
  };
  init_shards(master_shards_, 1);
  init_shards(slaves_shards_, 0);

  for (size_t i = 0; i < shards.size(); ++i) shards_[shards[i]] = i;

  sentinels_.state_change_callback = [this](Redis::State state) {
    LOG_TRACE() << "state=" << static_cast<int>(state);
    if (state != Redis::State::kInit) ev_async_send(loop_, &watch_state_);
  };
  sentinels_.ready_change_callback = [this](bool ready) {
    LOG_TRACE() << "ready=" << ready;
    if (ready) ev_async_send(loop_, &watch_create_);
  };
  for (const auto& conn : conns) {
    ConnectionInfoInt new_conn;
    static_cast<ConnectionInfo&>(new_conn) = conn;
    sentinels_.info.insert(new_conn);
  }

  Start();
}

SentinelImpl::~SentinelImpl() { Stop(); }

void SentinelImpl::InvokeCommand(CommandPtr command, ReplyPtr reply) {
  assert(reply);
  LOG_DEBUG()
      << "redis_request( " << reply->cmd
      << " ):" << (reply->status == REDIS_OK ? '+' : '-') << ":"
      << std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
             reply->time)
             .count();
  try {
    if (command->callback) command->callback(reply);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "exception in command->callback, cmd=" << reply->cmd << " "
                  << ex.what();
  }
}

void SentinelImpl::AsyncCommand(const SentinelCommand& scommand) {
  CommandPtr command = scommand.command;
  size_t shard = scommand.shard;
  bool master = scommand.master;
  LOG_TRACE() << shard;

  if (shard == unknown_shard) shard = 0;

  CommandPtr command_check_errors(PrepareCommand(
      command->args,
      [
        this, shard, command, master, start = scommand.start,
        counter = command->counter
      ](ReplyPtr reply) {
        if (counter != command->counter) return;
        assert(reply);

        std::chrono::steady_clock::time_point now =
            std::chrono::steady_clock::now();

        bool error_ask = reply->data.IsErrorAsk();
        bool error_moved = reply->data.IsErrorMoved();
        bool retry = reply->status != REDIS_OK || error_ask || error_moved;

        if (retry) {
          size_t new_shard = shard;
          if (error_ask || error_moved) {
            new_shard = ParseMovedShard(
                reply->data
                    .GetError());  // TODO: use correct host:port from shard
            command->counter++;
          }
          std::chrono::steady_clock::time_point until =
              start + command->control.timeout_all;
          if (now < until && command->control.max_retries > 1) {
            std::chrono::milliseconds timeout_all =
                std::chrono::duration_cast<std::chrono::milliseconds>(until -
                                                                      now);
            command->control = {
                std::min(command->control.timeout_single, timeout_all),
                timeout_all, command->control.max_retries - 1};
            command->counter++;
            command->asking = command->asking || error_ask;

            EnqueueCommand(SentinelCommand(
                command, master || (error_moved && shard == new_shard),
                new_shard, start));
            return;
          }
        }

        reply->time = now - start;
        InvokeCommand(command, reply);
      },
      command->control, command->counter, command->asking));

  if (!master) {
    std::shared_ptr<Shard> slaves_shard;
    assert(shard < slaves_shards_.size());
    slaves_shard = slaves_shards_[shard];
    if (slaves_shard->AsyncCommand(command_check_errors,
                                   &command->instance_idx))
      return;
  }
  std::shared_ptr<Shard> master_shard;
  assert(shard < master_shards_.size());
  master_shard = master_shards_[shard];
  if (!master_shard->AsyncCommand(command_check_errors,
                                  &command->instance_idx)) {
    EnqueueCommand(scommand);
    return;
  }
}

size_t SentinelImpl::ShardByKey(const std::string& key) const {
  assert(master_shards_.size() > 0);
  size_t shard = key_shard_ ? key_shard_->ShardByKey(key)
                            : slot_info_.ShardBySlot(HashSlot(key));
  if (!key_shard_ && shard == unknown_shard)
    LOG_ERROR() << "shard for slot " << HashSlot(key) << " (key '" << key
                << "') not found";
  return shard;
}

void SentinelImpl::Start() {
  check_timer_.data = this;
  ev_timer_init(&check_timer_, OnCheckTimer, 0.0, 0.0);
  TimerStart(check_timer_);

  watch_state_.data = this;
  ev_async_init(&watch_state_, ChangedState);
  AsyncStart(watch_state_);

  watch_update_.data = this;
  ev_async_init(&watch_update_, UpdateInstances);
  AsyncStart(watch_update_);

  watch_create_.data = this;
  ev_async_init(&watch_create_, OnModifyConnectionInfo);
  AsyncStart(watch_create_);

  watch_command_.data = this;
  ev_async_init(&watch_command_, CommandLoop);
  AsyncStart(watch_command_);
}

void SentinelImpl::Stop() {
  TimerStop(check_timer_);
  AsyncStop(watch_state_);
  AsyncStop(watch_update_);
  AsyncStop(watch_create_);
  AsyncStop(watch_command_);

  auto clean_shards = [](std::vector<std::shared_ptr<Shard>>& shards) {
    for (auto& shard : shards) shard->Clean();
  };
  {
    std::lock_guard<std::mutex> lock(command_mutex_);
    destroying_ = true;
    while (!commands_.empty()) {
      auto command = commands_.back().command;
      for (const auto& args : *command->args.args) {
        auto reply =
            std::make_shared<Reply>(args[0], nullptr, REDIS_ERR_NOT_READY);
        InvokeCommand(command, reply);
      }
      commands_.pop_back();
    }
  }
  clean_shards(master_shards_);
  clean_shards(slaves_shards_);
  sentinels_.Clean();
}

void SentinelImpl::OnCheckTimer(struct ev_loop*, ev_timer* w, int) {
  auto impl = static_cast<SentinelImpl*>(w->data);
  assert(impl != nullptr);
  impl->OnCheckTimerImpl();
}

void SentinelImpl::ChangedState(struct ev_loop*, ev_async* w, int) {
  auto impl = static_cast<SentinelImpl*>(w->data);
  assert(impl != nullptr);
  impl->CheckConnections();
}

inline void SentinelImpl::OnCheckTimerImpl() {
  auto process_shards = [this](bool& track,
                               std::vector<std::shared_ptr<Shard>>& shards) {
    if (!track) return;
    for (const auto& info : shards)
      info->ProcessCreation([this]() -> engine::ev::ThreadControl& {
        return thread_pool_.NextThread();
      });
  };

  try {
    sentinels_.ProcessCreation(
        [this]() -> engine::ev::ThreadControl& { return *this; });

    process_shards(track_masters_, master_shards_);
    process_shards(track_slaves_, slaves_shards_);

    ReadSentinels();
    if (!key_shard_)
      UpdateClusterSlots(master_shards_.empty()
                             ? 0
                             : ++current_slots_shard_ % master_shards_.size());
  } catch (const std::exception& ex) {
    LOG_ERROR() << "exception in check timer " << ex.what();
  }

  TimerStop(check_timer_);
  ev_timer_set(&check_timer_, check_interval_, 0.0);
  TimerStart(check_timer_);
}

void SentinelImpl::CommandLoop(struct ev_loop*, ev_async* w, int) {
  auto impl = static_cast<SentinelImpl*>(w->data);
  assert(impl != nullptr);
  impl->CommandLoopImpl();
}

inline void SentinelImpl::CommandLoopImpl() {
  std::vector<SentinelCommand> waiting_commands;

  // TODO: check shard availability

  {
    std::lock_guard<std::mutex> lock(command_mutex_);
    waiting_commands.swap(commands_);
  }

  for (const SentinelCommand& command : waiting_commands) AsyncCommand(command);
}

SentinelImpl::SlotInfo::SlotInfo() {
  std::vector<SlotInfo::ShardInterval> shard_intervals;
  shard_intervals.emplace_back(0, 16383, 0);
  UpdateSlots(shard_intervals);
}

size_t SentinelImpl::SlotInfo::ShardBySlot(size_t slot) const {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = std::upper_bound(
      slot_shards_.begin(), slot_shards_.end(), slot,
      [](const size_t& l, const SlotShard& r) { return l < r.bound; });
  if (it == slot_shards_.begin()) return unknown_shard;
  --it;
  return it->shard;
}

void SentinelImpl::SlotInfo::UpdateSlots(
    const std::vector<ShardInterval>& intervals) {
  std::vector<size_t> slot_bounds;
  slot_bounds.reserve(intervals.size() * 2);
  for (const ShardInterval& interval : intervals) {
    if (interval.shard == unknown_shard) continue;
    slot_bounds.push_back(interval.slot_min);
    slot_bounds.push_back(interval.slot_max + 1);
  }
  std::sort(slot_bounds.begin(), slot_bounds.end());
  slot_bounds.erase(std::unique(slot_bounds.begin(), slot_bounds.end()),
                    slot_bounds.end());

  auto __attribute__((unused)) check_intervals = [&intervals, &slot_bounds]() {
    for (const ShardInterval& interval : intervals) {
      size_t idx = std::lower_bound(slot_bounds.begin(), slot_bounds.end(),
                                    interval.slot_min) -
                   slot_bounds.begin();
      if (idx + 1 >= slot_bounds.size() ||
          slot_bounds[idx] != interval.slot_min ||
          slot_bounds[idx + 1] != interval.slot_max + 1) {
        std::ostringstream os;
        os << "wrong intervals in " << __func__ << ": [";
        bool f = true;
        for (const ShardInterval& i : intervals) {
          if (f)
            f = false;
          else
            os << ", ";
          os << "{ slot_min = " << i.slot_min << ", "
             << "slot_max = " << i.slot_max << ", "
             << "shard = " << i.shard << "}";
        }
        os << "]";
        LOG_ERROR() << os.str();
        return false;
      }
    }
    return true;
  };
  assert(check_intervals());

  std::vector<SlotShard> slot_shards_new;
  slot_shards_new.reserve(slot_bounds.size());
  size_t local_unknown_shard = unknown_shard;
  for (const size_t& bound : slot_bounds)
    slot_shards_new.emplace_back(bound, local_unknown_shard);
  for (const ShardInterval& interval : intervals) {
    if (interval.shard == unknown_shard) continue;
    const auto it = std::lower_bound(
        slot_shards_new.begin(), slot_shards_new.end(), interval.slot_min,
        [](const SlotShard& l, const size_t& r) { return l.bound < r; });
    assert(it != slot_shards_new.end() && it->bound == interval.slot_min);
    it->shard = interval.shard;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  slot_shards_.swap(slot_shards_new);
}

size_t SentinelImpl::ShardInfo::GetShard(const std::string& host,
                                         int port) const {
  std::lock_guard<std::mutex> lock(mutex_);

  const auto it = host_port_to_shard.find(std::make_pair(host, port));
  if (it == host_port_to_shard.end()) return unknown_shard;
  return it->second;
}

void SentinelImpl::ShardInfo::UpdateHostPortToShard(
    HostPortToShardMap&& host_port_to_shard_new) {
  bool changed;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    changed = host_port_to_shard_new != host_port_to_shard;
  }

  if (changed) {
    std::lock_guard<std::mutex> lock(mutex_);

    host_port_to_shard.swap(host_port_to_shard_new);
  }
}

bool SentinelImpl::Shard::AsyncCommand(CommandPtr command,
                                       size_t* pinstance_idx /* = nullptr*/) {
  if (destroying) return false;
  std::shared_ptr<Redis> instance;
  for (size_t attempt = 0, attempts = 0;; attempt++) {
    {
      std::shared_lock<std::shared_timed_mutex> lock(mutex);
      if (!attempts) attempts = instances.size() + 1;
      if (attempt >= attempts) return false;
      size_t skip_idx = !attempt && pinstance_idx ? *pinstance_idx : -1;
      for (size_t i = 0; i < instances.size(); i++) {
        size_t instance_idx = (current + i) % instances.size();
        if (instance_idx == skip_idx) continue;
        const auto& cur_inst = instances[instance_idx].instance;
        if (!cur_inst->IsDestroying() &&
            (!instance || instance->IsDestroying() ||
             cur_inst->GetRunningCommands() < instance->GetRunningCommands())) {
          if (pinstance_idx) *pinstance_idx = instance_idx;
          instance = cur_inst;
        }
      }
      ++current;
    }
    if (instance && instance->AsyncCommand(command)) return true;
  }
  return false;
}

void SentinelImpl::Shard::Clean() {
  std::vector<ConnectionStatus> local_instances;
  std::vector<ConnectionStatus> local_clean_wait;

  {
    std::unique_lock<std::shared_timed_mutex> lock(mutex);
    destroying = true;
    local_instances.swap(instances);
    local_clean_wait.swap(clean_wait);
  }
}

void SentinelImpl::EnqueueCommand(const SentinelCommand& scommand) {
  {
    std::lock_guard<std::mutex> lock(command_mutex_);
    if (destroying_) {
      auto& command = scommand.command;
      for (const auto& args : *command->args.args) {
        auto reply =
            std::make_shared<Reply>(args[0], nullptr, REDIS_ERR_NOT_READY);
        InvokeCommand(command, reply);
      }
      return;
    }
    commands_.push_back(scommand);
  }
  ev_async_send(loop_, &watch_command_);
}

size_t SentinelImpl::ParseMovedShard(const std::string& err_string) {
  size_t pos = err_string.find(' ');  // skip "MOVED" or "ASK"
  if (pos == std::string::npos) return unknown_shard;
  pos = err_string.find(' ', pos + 1);  // skip hash_slot
  if (pos == std::string::npos) return unknown_shard;
  pos++;
  size_t end = err_string.find(' ', pos);
  if (end == std::string::npos) end = err_string.size();
  size_t colon_pos = err_string.rfind(':', end);
  int port;
  try {
    port = std::stol(err_string.substr(colon_pos + 1, end - (colon_pos + 1)));
  } catch (const std::exception& ex) {
    LOG_ERROR() << "exception while parsing port from (\"" << err_string
                << "\") " << ex.what();
    return unknown_shard;
  }
  std::string host = err_string.substr(pos, colon_pos - pos);
  return shard_info_.GetShard(host, port);
}

void SentinelImpl::UpdateClusterSlots(size_t shard) {
  CommandPtr command = PrepareCommand(
      CmdArgs{"CLUSTER", "SLOTS"},
      [this](ReplyPtr reply) {
        assert(reply);
        if (!reply->data.IsArray()) {
          LOG_WARNING() << "reply to 'CLUSTER SLOTS' is not array ("
                        << static_cast<int>(reply->data.GetType())
                        << "): " << reply->data.ToString();
          return;
        }
        std::vector<SlotInfo::ShardInterval> shard_intervals;
        shard_intervals.reserve(reply->data.GetArray().size());
        for (const ReplyData& reply_interval : reply->data.GetArray()) {
          if (!reply_interval.IsArray()) continue;
          const std::vector<ReplyData>& array = reply_interval.GetArray();
          if (array.size() < 3) continue;
          if (!array[0].IsInt() || !array[1].IsInt()) continue;
          for (size_t i = 2; i < array.size(); i++) {
            if (!array[i].IsArray()) continue;
            const std::vector<ReplyData>& host_info_array = array[i].GetArray();
            if (host_info_array.size() < 2) continue;
            if (!host_info_array[0].IsString() || !host_info_array[1].IsInt())
              continue;
            size_t shard = shard_info_.GetShard(host_info_array[0].GetString(),
                                                host_info_array[1].GetInt());
            if (shard != unknown_shard) {
              shard_intervals.emplace_back(array[0].GetInt(), array[1].GetInt(),
                                           shard);
              break;
            }
          }
        }
        slot_info_.UpdateSlots(shard_intervals);
      },
      {cluster_slots_timeout_, cluster_slots_timeout_, 1});
  std::shared_ptr<Shard> master;
  if (shard < master_shards_.size()) master = master_shards_[shard];
  if (master) master->AsyncCommand(command);
}

size_t SentinelImpl::HashSlot(const std::string& key) {
  size_t start, len;
  GetRedisKey(key, start, len);
  return std::for_each(key.data() + start, key.data() + start + len,
                       boost::crc_optimal<16, 0x1021>())() &
         0x3fff;
}

void SentinelImpl::SentinelGetHosts(
    CmdArgs&& command, bool allow_empty,
    std::function<void(const ConnInfoByShard& info)>&& callback) {
  sentinels_.AsyncCommand(PrepareCommand(
      std::move(command),
      [ this, callback = std::move(callback), allow_empty ](ReplyPtr reply) {
        SentinelImpl::ConnInfoByShard res;
        try {
          std::vector<std::map<std::string, std::string>> conns;
          bool resp_ok = ParseSentinelResponse(reply->data, reply->status,
                                               allow_empty, conns);
          if (resp_ok) {
            for (auto& properties : conns) {
              if (SentinelNodeIsActive(properties)) {
                SentinelImpl::ConnectionInfoInt info;
                info.name = properties["name"];
                info.host = properties["ip"];
                info.port = std::stol(properties["port"]);
                info.password = password_;
                LOG_TRACE() << "name=" << info.name << " host=" << info.host
                            << " port=" << info.port;
                res.push_back(info);
              }
            }
            callback(res);
          }
        } catch (const std::exception& ex) {
          LOG_ERROR() << "exception in SentinelGetHosts():"
                      << " reply was '" << reply->data.ToString() << "'"
                      << " type = " << static_cast<int>(reply->data.GetType())
                      << ". " << ex.what();
        }
      }));
}

void SentinelImpl::ReadSentinels() {
  SentinelGetHosts(
      CmdArgs{"SENTINEL", "MASTERS"}, false,
      [this](const ConnInfoByShard& info) {
        struct WatchContext {
          ConnInfoByShard masters;
          ConnInfoByShard slaves;
          std::mutex mutex;
          int counter;
          ShardInfo::HostPortToShardMap host_port_to_shard;
        };
        auto watcher = std::make_shared<WatchContext>();

        for (const auto& shard_conn : info) {
          if (shards_.find(shard_conn.name) != shards_.end()) {
            LOG_TRACE() << "name=" << shard_conn.name
                        << " host=" << shard_conn.host
                        << " port=" << shard_conn.port;
            watcher->masters.push_back(shard_conn);
            watcher->host_port_to_shard[std::make_pair(
                shard_conn.host, shard_conn.port)] = shards_[shard_conn.name];
          }
        }

        watcher->counter = watcher->masters.size();

        for (const auto& shard_conn : watcher->masters) {
          SentinelGetHosts(
              CmdArgs{"SENTINEL", "SLAVES", shard_conn.name}, true,
              [ this, watcher,
                shard = shard_conn.name ](const ConnInfoByShard& info) {
                std::lock_guard<std::mutex> lock(watcher->mutex);
                for (auto shard_conn : info) {
                  shard_conn.name = shard;
                  if (shards_.find(shard_conn.name) != shards_.end())
                    watcher->host_port_to_shard[std::make_pair(
                        shard_conn.host, shard_conn.port)] =
                        shards_[shard_conn.name];
                  watcher->slaves.push_back(shard_conn);
                }
                if (!--watcher->counter) {
                  shard_info_.UpdateHostPortToShard(
                      std::move(watcher->host_port_to_shard));

                  {
                    std::lock_guard<std::mutex> lock_this(sentinels_mutex_);
                    master_shards_info_.swap(watcher->masters);
                    slaves_shards_info_.swap(watcher->slaves);
                  }
                  ev_async_send(loop_, &watch_update_);
                }
              });
        }
      });
}

void SentinelImpl::UpdateInstances(struct ev_loop*, ev_async* w, int) {
  auto impl = static_cast<SentinelImpl*>(w->data);
  assert(impl != nullptr);
  impl->UpdateInstancesImpl();
}

inline SentinelImpl::ConnInfoMap SentinelImpl::ConvertConnectionInfoVectorToMap(
    const std::vector<ConnectionInfoInt>& array) {
  ConnInfoMap res;
  for (const auto& info : array) res[info.name].push_back(info);
  return res;
}

inline bool SentinelImpl::SetConnectionInfo(
    const SentinelImpl::ConnInfoMap& info_by_shards,
    std::vector<std::shared_ptr<Shard>>& shards) {
  bool res = false;
  for (auto info_iterator : info_by_shards) {
    auto j = shards_.find(info_iterator.first);
    if (j != shards_.end()) {
      res |= shards[j->second]->SetConnectionInfo(
          info_iterator.second, [this](Redis::State state) {
            if (state != Redis::State::kInit && loop_)
              ev_async_send(loop_, &watch_state_);
          });
    }
  }
  return res;
}

void SentinelImpl::UpdateInstancesImpl() {
  bool changed = false;
  {
    std::lock_guard<std::mutex> lock(sentinels_mutex_);
    auto master_map = ConvertConnectionInfoVectorToMap(master_shards_info_);
    auto slaves_map = ConvertConnectionInfoVectorToMap(slaves_shards_info_);
    changed |= SetConnectionInfo(master_map, master_shards_);
    changed |= SetConnectionInfo(slaves_map, slaves_shards_);
  }
  if (changed) ev_async_send(loop_, &watch_create_);
}

void SentinelImpl::OnModifyConnectionInfo(struct ev_loop*, ev_async* w, int) {
  auto impl = static_cast<SentinelImpl*>(w->data);
  assert(impl != nullptr);
  impl->OnCheckTimerImpl();
}

void SentinelImpl::CheckConnections() {
  sentinels_.ProcessStateUpdate();
  auto process_shards = [](const std::vector<std::shared_ptr<Shard>>& shards) {
    for (auto& info : shards) info->ProcessStateUpdate();
  };

  process_shards(master_shards_);
  process_shards(slaves_shards_);

  std::vector<SentinelCommand> waiting_commands;

  {
    std::lock_guard<std::mutex> lock(command_mutex_);
    waiting_commands.swap(commands_);
  }

  for (const SentinelCommand& command : waiting_commands) AsyncCommand(command);
}

inline void SentinelImpl::Shard::ProcessCreation(
    const std::function<engine::ev::ThreadControl&()>& get_thread) {
  std::vector<ConnectionStatus> erase_instance;

  {
    std::unique_lock<std::shared_timed_mutex> lock(mutex);

    std::set<ConnectionInfoInt> need_to_create(info);

    for (const auto& instance : instances) need_to_create.erase(instance.info);
    for (const auto& instance : clean_wait) need_to_create.erase(instance.info);

    for (const auto& id : need_to_create) {
      ConnectionStatus entry;
      entry.info = id;
      entry.instance = std::make_shared<Redis>(get_thread(), read_only);
      LOG_TRACE() << "create instance";
      entry.instance->Connect(entry.info, state_change_callback);
      clean_wait.push_back(entry);
    }

    for (auto instance_iterator = instances.begin();
         instance_iterator != instances.end();) {
      if (info.find(instance_iterator->info) == info.end()) {
        erase_instance.emplace_back(std::move(*instance_iterator));
        instance_iterator = instances.erase(instance_iterator);
      } else
        ++instance_iterator;
    }
  }
}

inline void SentinelImpl::Shard::ProcessStateUpdate() {
  std::vector<ConnectionStatus> erase_clean_wait;
  bool new_connected;

  {
    std::unique_lock<std::shared_timed_mutex> lock(mutex);
    for (auto info = instances.begin(); info != instances.end();) {
      if (info->instance->GetState() != Redis::State::kConnected) {
        clean_wait.emplace_back(std::move(*info));
        info = instances.erase(info);
      } else
        ++info;
    }
    for (auto info = clean_wait.begin(); info != clean_wait.end();) {
      switch (info->instance->GetState()) {
        case Redis::State::kConnected:
          instances.emplace_back(std::move(*info));
          info = clean_wait.erase(info);
          break;
        case Redis::State::kConnectError:
        case Redis::State::kConnectHiredisError:
        case Redis::State::kInitError:
        case Redis::State::kDisconnected:
        case Redis::State::kExitReady:
          erase_clean_wait.emplace_back(std::move(*info));
          info = clean_wait.erase(info);
          break;
        case Redis::State::kInit:
        default:
          ++info;
          break;
      }
    }
    new_connected = (instances.size() > 0);
  }

  LOG_TRACE() << "prev_connected=" << prev_connected
              << " new_connected=" << new_connected;
  if (prev_connected != new_connected) {
    if (ready_change_callback) ready_change_callback(new_connected);
    prev_connected = new_connected;
  }
}

inline bool SentinelImpl::Shard::SetConnectionInfo(
    const std::vector<ConnectionInfoInt>& info_array,
    std::function<void(Redis::State state)> callback) {
  std::set<ConnectionInfoInt> new_info;
  for (const auto& info_entry : info_array) new_info.insert(info_entry);
  std::unique_lock<std::shared_timed_mutex> lock(mutex);
  if (new_info == info) return false;
  state_change_callback = std::move(callback);
  std::swap(info, new_info);
  return true;
}

std::vector<Stat> SentinelImpl::Shard::GetStat() {
  std::shared_lock<std::shared_timed_mutex> lock(mutex);
  std::vector<Stat> res;
  for (const auto& instance : instances)
    res.push_back(instance.instance->GetStat());
  return res;
}

Stat SentinelImpl::GetStat() {
  Stat res;
  std::vector<Stat> acc;
  auto accumulate = [](std::vector<std::shared_ptr<Shard>>& shards,
                       std::vector<Stat>& acc) {
    for (auto& shard : shards) {
      auto stat = shard->GetStat();
      acc.insert(acc.end(), stat.begin(), stat.end());
    }
  };
  accumulate(master_shards_, acc);
  accumulate(slaves_shards_, acc);
  for (auto& stat : acc) {
    res.tps += stat.tps;
    res.queue += stat.queue;
    res.inprogress += stat.inprogress;
    res.timeouts += stat.timeouts;
  }
  return res;
}

constexpr const std::chrono::milliseconds SentinelImpl::cluster_slots_timeout_;

}  // namespace redis
}  // namespace storages
