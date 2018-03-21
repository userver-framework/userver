#include "redis.hpp"

#include <chrono>
#include <mutex>

#include <hiredis/adapters/libev.h>
#include <hiredis/hiredis.h>
#include <boost/circular_buffer.hpp>

#include <logging/logger.hpp>

namespace storages {
namespace redis {
namespace {

template <typename Rep, typename Period>
double ToEvDuration(const std::chrono::duration<Rep, Period>& duration) {
  return std::chrono::duration_cast<std::chrono::duration<double>>(duration)
      .count();
}

}  // namespace

class RedisImpl : public engine::ev::ThreadControl {
 public:
  using State = Redis::State;

  RedisImpl(const engine::ev::ThreadControl& thread_control,
            bool read_only = false);
  ~RedisImpl();

  void Attach();
  void Detach();

  void Connect(const std::string& host, int port, const std::string& password,
               std::function<void(State state)> callback);
  void Disconnect();

  bool AsyncCommand(CommandPtr command);

  State GetState();
  Stat GetStat();
  size_t GetRunningCommands() const;
  bool IsDestroying() const { return destroying_; }

 private:
  static void CommandLoop(struct ev_loop* loop, ev_async* w, int revents);
  static void OnStatUpdate(struct ev_loop* loop, ev_timer* w, int revents);
  static void OnCommandTimeout(struct ev_loop* loop, ev_timer* w, int revents);

  static inline void InvokeCommand(CommandPtr command, ReplyPtr reply);

  void CommandLoopImpl();
  void OnStatUpdateImpl();
  void OnCommandTimeoutImpl(ev_timer* w);

  void SetState(State state);
  void ProcessCommand(std::shared_ptr<Command> command);

  void Authenticate();
  void FreeCommands();

 private:
  using CommandRef = std::shared_ptr<Command>;

  class ReplyPrivdatum;

  struct SingleCommand {
    std::string cmd;
    CommandRef meta;
    ev_timer timer;
    std::shared_ptr<ReplyPrivdatum> reply_privdatum;
    bool invoked = false;
  };

  class ReplyPrivdatum : public std::enable_shared_from_this<ReplyPrivdatum> {
   public:
    ReplyPrivdatum(RedisImpl* redis_impl) : redis_impl(redis_impl) {}

    ~ReplyPrivdatum() {}

    static void OnRedisReply(redisAsyncContext* c, void* r, void* privdata);
    static void OnConnect(const redisAsyncContext* c, int status);
    static void OnDisconnect(const redisAsyncContext* c, int status);
    static void OnTimerPing(struct ev_loop* loop, ev_timer* w, int revents);

    void OnRedisReplyImpl(redisReply* r, void* privdata);
    void OnConnectImpl(int status);
    void OnDisconnectImpl();
    void OnTimerPingImpl();

    std::unordered_map<size_t, std::unique_ptr<SingleCommand>> reply_privdata;
    std::unordered_map<const ev_timer*, size_t> reply_privdata_rev;
    RedisImpl* redis_impl = nullptr;
    bool subscriber_ = false;
    std::shared_ptr<ReplyPrivdatum> self;
  };

  friend class ReplyPrivdatum;

  struct AccStat {
    size_t recv = 0;
    size_t sent = 0;
    size_t queue = 0;
    size_t timeouts = 0;
    std::chrono::steady_clock::time_point ts =
        std::chrono::steady_clock::time_point::min();

    AccStat() {}
    AccStat(size_t recv, size_t sent, size_t queue, size_t timeouts,
            std::chrono::steady_clock::time_point ts)
        : recv(recv), sent(sent), queue(queue), timeouts(timeouts), ts(ts) {}
  };

  bool SetDestroying() {
    std::lock_guard<std::mutex> lock(command_mutex_);
    if (destroying_) return false;
    destroying_ = true;
    return true;
  }

  std::mutex command_mutex_;
  std::deque<CommandPtr> commands_;
  redisAsyncContext* context_ = nullptr;
  State state_ = State::kInit;
  std::function<void(State state)> callback_;
  std::string host_;
  int port_;
  std::string password_;
  size_t queue_count_ = 0;
  size_t sent_count_ = 0;
  size_t timeout_count_ = 0;
  size_t complete_count_ = 0;
  size_t cmd_counter_ = 0;
  std::shared_ptr<ReplyPrivdatum> reply_privdatum_;
  bool subscriber_ = false;
  struct ev_loop* loop_ = nullptr;
  ev_timer ping_timer_;
  ev_timer stat_timer_;
  ev_async watch_command_;
  std::chrono::milliseconds ping_interval_{2000};
  std::chrono::milliseconds ping_timeout_{4000};
  std::chrono::milliseconds stat_interval_{500};
  std::mutex stat_mutex_;
  boost::circular_buffer<AccStat> stat_;
  Stat stat_res_;
  bool destroying_ = false;
  size_t cycles_ = 0;
  size_t old_cycles_ = 0;
  bool read_only_ = false;
  bool attached_ = false;
};

Redis::Redis(const engine::ev::ThreadControl& thread_control, bool read_only)
    : engine::ev::ImplInEvLoop<RedisImpl>(thread_control, read_only) {}

Redis::~Redis() {}

void Redis::Connect(const ConnectionInfo& conn,
                    std::function<void(State state)> callback) {
  impl_->Connect(conn.host, conn.port, conn.password, callback);
}

void Redis::Connect(const std::string& host, int port,
                    const std::string& password,
                    std::function<void(State state)> callback) {
  impl_->Connect(host, port, password, callback);
}

bool Redis::AsyncCommand(CommandPtr command) {
  return impl_->AsyncCommand(command);
}

Redis::State Redis::GetState() { return impl_->GetState(); }

Stat Redis::GetStat() { return impl_->GetStat(); }

size_t Redis::GetRunningCommands() const { return impl_->GetRunningCommands(); }

bool Redis::IsDestroying() const { return impl_->IsDestroying(); }

RedisImpl::RedisImpl(const engine::ev::ThreadControl& thread_control,
                     bool read_only)
    : engine::ev::ThreadControl(thread_control),
      reply_privdatum_(std::make_shared<ReplyPrivdatum>(this)),
      loop_(GetEvLoop()),
      read_only_(read_only) {
  assert(loop_);
  stat_.set_capacity(30);
}

void RedisImpl::Attach() {
  stat_timer_.data = this;
  ev_timer_init(&stat_timer_, OnStatUpdate, ToEvDuration(stat_interval_), 0.0);
  TimerStart(stat_timer_);

  // start after connecting
  watch_command_.data = this;
  ev_async_init(&watch_command_, CommandLoop);

  ping_timer_.data = reply_privdatum_.get();
  ev_timer_init(&ping_timer_, ReplyPrivdatum::OnTimerPing,
                ToEvDuration(ping_interval_), 0.0);

  attached_ = true;
}

void RedisImpl::Detach() {
  if (!attached_) return;

  AsyncStop(watch_command_);
  TimerStop(ping_timer_);
  TimerStop(stat_timer_);

  attached_ = false;
}

RedisImpl::~RedisImpl() {
  Disconnect();
  reply_privdatum_->redis_impl = nullptr;
  assert(context_ == nullptr);
}

void RedisImpl::Connect(const std::string& host, int port,
                        const std::string& password,
                        std::function<void(State state)> callback) {
  assert(callback);
  assert(context_ == nullptr);
  assert(state_ == State::kInit);

  callback_ = std::move(callback);
  host_ = host;
  port_ = port;
  password_ = password;
  context_ = redisAsyncConnect(host_.c_str(), port_);

  assert(context_ != nullptr);

  context_->data = reply_privdatum_.get();

  if (context_->err) {
    LOG_ERROR() << "error after redisAsyncConnect (host=" << host
                << ", port=" << port << "): " << context_->errstr;
    redisAsyncFree(context_);
    context_ = nullptr;
    SetState(State::kInitError);
  } else {
    bool err = (loop_ == nullptr);
    if (!err) Attach();
    RunInEvLoopSync([this, &err]() {
      if (!err) err = redisLibevAttach(loop_, context_) != REDIS_OK;
      if (!err)
        err = redisAsyncSetConnectCallback(
                  context_, ReplyPrivdatum::OnConnect) != REDIS_OK;
      if (!err)
        err = redisAsyncSetDisconnectCallback(
                  context_, ReplyPrivdatum::OnDisconnect) != REDIS_OK;
      SetState(err ? State::kInitError : State::kInit);
    });
  }
}

void RedisImpl::Disconnect() {
  if (!SetDestroying()) return;

  Detach();

  if (state_ == State::kInit || state_ == State::kConnected ||
      state_ == State::kConnectError)
    redisAsyncDisconnect(context_);

  FreeCommands();
  context_ = nullptr;

  SetState(State::kExitReady);
}

inline void RedisImpl::InvokeCommand(CommandPtr command, ReplyPtr reply) {
  assert(reply);
  try {
    LOG_TRACE() << "reply=" << reply->data.ToString();
    if (command->callback) command->callback(reply);
  } catch (const std::exception& ex) {
    LOG_ERROR() << "exception in callback handler (" << command->args.ToString()
                << ") " << ex.what();
  }
}

bool RedisImpl::AsyncCommand(CommandPtr command) {
  LOG_TRACE() << (*command->args.args)[0][0];
  {
    std::lock_guard<std::mutex> lock(command_mutex_);
    if (destroying_) return false;
    ++queue_count_;
    commands_.push_back(command);
  }
  ev_async_send(loop_, &watch_command_);
  return true;
}

void RedisImpl::ReplyPrivdatum::OnTimerPing(struct ev_loop*, ev_timer* w, int) {
  auto impl = static_cast<RedisImpl::ReplyPrivdatum*>(w->data);
  assert(impl != nullptr);
  impl->OnTimerPingImpl();
}

inline void RedisImpl::ReplyPrivdatum::OnTimerPingImpl() {
  if (!redis_impl) return;
  redis_impl->TimerStop(redis_impl->ping_timer_);
  if (redis_impl->subscriber_) return;
  redis_impl->ProcessCommand(PrepareCommand(
      CmdArgs{"PING"},
      [this](ReplyPtr reply) {
        if (!redis_impl) return;
        if (reply->status == REDIS_OK) {
          ev_timer_set(&redis_impl->ping_timer_,
                       ToEvDuration(redis_impl->ping_interval_), 0.0);
          redis_impl->TimerStart(redis_impl->ping_timer_);
        } else
          redis_impl->Disconnect();
      },
      CommandControl{redis_impl->ping_timeout_, redis_impl->ping_timeout_, 1}));
}

void RedisImpl::OnCommandTimeout(struct ev_loop*, ev_timer* w, int) {
  auto impl = static_cast<RedisImpl*>(w->data);
  assert(impl != nullptr);
  impl->OnCommandTimeoutImpl(w);
}

void RedisImpl::OnCommandTimeoutImpl(ev_timer* w) {
  SingleCommand* command = nullptr;

  auto& reply_privdata_rev = reply_privdatum_->reply_privdata_rev;
  auto& reply_privdata = reply_privdatum_->reply_privdata;
  size_t cmd_idx = reply_privdata_rev.at(w);
  auto reply_iterator = reply_privdata.find(cmd_idx);
  if (reply_iterator != reply_privdata.end()) {
    command = reply_iterator->second.get();
    if (!subscriber_) --sent_count_;
    ++timeout_count_;
    ++complete_count_;
    assert(reply_privdata_rev.count(&command->timer));
    assert(w == &command->timer);
    reply_privdata_rev.erase(&command->timer);
    command->invoked = true;
  }

  if (command) {
    auto reply =
        std::make_shared<Reply>(command->cmd, nullptr, REDIS_ERR_TIMEOUT);
    InvokeCommand(command->meta, reply);
  }
}

void RedisImpl::OnStatUpdate(struct ev_loop*, ev_timer* w, int) {
  auto impl = static_cast<RedisImpl*>(w->data);
  assert(impl != nullptr);
  impl->OnStatUpdateImpl();
}

void RedisImpl::OnStatUpdateImpl() {
  TimerStop(stat_timer_);
  ev_timer_set(&stat_timer_, ToEvDuration(stat_interval_), 0.0);
  TimerStart(stat_timer_);

  stat_mutex_.lock();
  stat_.push_back(AccStat(complete_count_, sent_count_, queue_count_,
                          timeout_count_, std::chrono::steady_clock::now()));
  AccStat min, max;
  size_t queue = 0;
  size_t inprogress = 0;
  size_t count = 0;
  size_t timeouts = 0;
  for (auto& entry : stat_) {
    if (entry.ts == std::chrono::steady_clock::time_point::min()) continue;
    queue += entry.queue;
    inprogress += entry.sent;
    timeouts += entry.timeouts;
    ++count;
    if (min.ts == std::chrono::steady_clock::time_point::min()) min = entry;
    if (max.ts == std::chrono::steady_clock::time_point::min()) max = entry;
    if (entry.ts < min.ts) min = entry;
    if (entry.ts > max.ts) max = entry;
  }
  if (min.ts != max.ts) {
    auto delta =
        std::chrono::duration_cast<std::chrono::microseconds>(max.ts - min.ts)
            .count();
    stat_res_.tps =
        (max.recv - min.recv) / static_cast<double>(delta) * 1000000.0;
    stat_res_.queue = queue;
    stat_res_.inprogress = inprogress;
    stat_res_.timeouts = timeouts;
  }
  stat_mutex_.unlock();
}

Redis::State RedisImpl::GetState() { return state_; }

Stat RedisImpl::GetStat() {
  std::lock_guard<std::mutex> lock(stat_mutex_);
  return stat_res_;
}

size_t RedisImpl::GetRunningCommands() const { return sent_count_; }

inline void RedisImpl::SetState(State state) {
  if (state != state_) {
    state_ = state;

    if (state == State::kConnected) {
      if (redisEnableKeepAlive(&context_->c) != REDIS_OK) {
        Disconnect();
      } else {
        AsyncStart(watch_command_);
        TimerStart(ping_timer_);
      }
    } else if (state == State::kInitError || state == State::kConnectError ||
               state == State::kConnectHiredisError ||
               state == State::kDisconnected)
      Disconnect();

    callback_(state);
  }
}

void RedisImpl::FreeCommands() {
  while (!commands_.empty()) {
    auto command = commands_.front();
    commands_.pop_front();
    --queue_count_;
    for (const auto& args : *command->args.args)
      InvokeCommand(command, std::make_shared<Reply>(args[0], nullptr,
                                                     REDIS_ERR_NOT_READY));
  }
  for (auto& info : reply_privdatum_->reply_privdata)
    TimerStop(info.second->timer);
}

void RedisImpl::CommandLoop(struct ev_loop*, ev_async* w, int) {
  auto impl = static_cast<RedisImpl*>(w->data);
  assert(impl != nullptr);
  impl->CommandLoopImpl();
}

inline void RedisImpl::CommandLoopImpl() {
  while (true) {
    std::shared_ptr<Command> command;
    {
      std::lock_guard<std::mutex> lock(command_mutex_);
      if (commands_.empty()) break;
      command = commands_.front();
      commands_.pop_front();
      --queue_count_;
    }
    ProcessCommand(command);
  }
}

void RedisImpl::ReplyPrivdatum::OnConnect(const redisAsyncContext* c,
                                          int status) {
  auto impl = static_cast<RedisImpl::ReplyPrivdatum*>(c->data);
  assert(impl != nullptr);
  impl->OnConnectImpl(status);
}

void RedisImpl::ReplyPrivdatum::OnDisconnect(const redisAsyncContext* c, int) {
  auto impl = static_cast<RedisImpl::ReplyPrivdatum*>(c->data);
  assert(impl != nullptr);
  impl->OnDisconnectImpl();
}

void RedisImpl::ReplyPrivdatum::OnConnectImpl(int status) {
  if (!redis_impl) return;
  if (status == REDIS_OK) {
    redis_impl->Authenticate();
    self = shared_from_this();
  } else {
    LOG_TRACE() << "connect_hiredis_error";
    redis_impl->SetState(State::kConnectHiredisError);
  }
}

void RedisImpl::ReplyPrivdatum::OnDisconnectImpl() {
  if (redis_impl) redis_impl->SetState(State::kDisconnected);
  self = nullptr;
}

void RedisImpl::Authenticate() {
  auto check_read_only = [this]() {
    if (read_only_) {
      ProcessCommand(
          PrepareCommand(CmdArgs{"READONLY"}, [this](ReplyPtr reply) {
            SetState(reply->status == REDIS_OK ? State::kConnected
                                               : State::kConnectError);
          }));
    } else
      SetState(State::kConnected);
  };

  if (password_.empty())
    check_read_only();
  else {
    ProcessCommand(PrepareCommand(
        CmdArgs{"AUTH", password_}, [this, check_read_only](ReplyPtr reply) {
          if (reply->status == REDIS_OK && reply->data.IsStatus() &&
              reply->data.ToString() == "OK")
            check_read_only();
          else
            SetState(State::kConnectError);
        }));
  }
}

void RedisImpl::ReplyPrivdatum::OnRedisReply(redisAsyncContext* c, void* r,
                                             void* privdata) {
  auto impl = static_cast<RedisImpl::ReplyPrivdatum*>(c->data);
  assert(impl != nullptr);
  impl->OnRedisReplyImpl(static_cast<redisReply*>(r), privdata);
}

inline void RedisImpl::ReplyPrivdatum::OnRedisReplyImpl(redisReply* redis_reply,
                                                        void* privdata) {
  std::unique_ptr<SingleCommand> command;

  auto data = reply_privdata.find(reinterpret_cast<size_t>(privdata));
  if (data != reply_privdata.end()) {
    if (redis_impl) ++redis_impl->complete_count_;
    if (subscriber_) {
      if (redis_impl) redis_impl->TimerStop(data->second->timer);
    } else {
      command = std::move(data->second);
      if (redis_impl) {
        --redis_impl->sent_count_;
        redis_impl->TimerStop(command->timer);
      }

      if (!command->invoked) {
        assert(reply_privdata_rev.count(&command->timer));
        assert(reply_privdata_rev.at(&command->timer) ==
               reinterpret_cast<size_t>(privdata));
        reply_privdata_rev.erase(&command->timer);
      }
      reply_privdata.erase(data);
    }
  }

  if (command && !command->invoked) {
    auto reply = std::make_shared<Reply>(command->cmd, redis_reply, REDIS_OK);
    RedisImpl::InvokeCommand(command->meta, reply);
  }
}

inline bool IsUnsubscribeCommand(const CmdArgs::CmdArgsArray& args) {
  static const std::string unsubscribe_command{"UNSUBSCRIBE"};
  static const std::string punsubscribe_command{"PUNSUBSCRIBE"};

  if (args[0].size() == unsubscribe_command.size() ||
      args[0].size() == punsubscribe_command.size()) {
    return !strcasecmp(unsubscribe_command.c_str(), args[0].c_str()) ||
           !strcasecmp(punsubscribe_command.c_str(), args[0].c_str());
  }

  return false;
}

inline bool IsSubscribeCommand(const CmdArgs::CmdArgsArray& args) {
  static const std::string subscribe_command{"SUBSCRIBE"};
  static const std::string psubscribe_command{"PSUBSCRIBE"};

  if (args[0].size() == subscribe_command.size() ||
      args[0].size() == psubscribe_command.size()) {
    return !strcasecmp(subscribe_command.c_str(), args[0].c_str()) ||
           !strcasecmp(psubscribe_command.c_str(), args[0].c_str());
  }

  return false;
}

inline bool IsSubscribesCommand(const CmdArgs::CmdArgsArray& args) {
  return IsSubscribeCommand(args) || IsUnsubscribeCommand(args);
}

inline bool IsMultiCommand(const CmdArgs::CmdArgsArray& args) {
  static const std::string multi_command{"MULTI"};

  if (args[0].size() == multi_command.size())
    return !strcasecmp(multi_command.c_str(), args[0].c_str());

  return false;
}

inline bool IsExecCommand(const CmdArgs::CmdArgsArray& args) {
  static const std::string exec_command{"EXEC"};

  if (args[0].size() == exec_command.size())
    return !strcasecmp(exec_command.c_str(), args[0].c_str());

  return false;
}

inline void RedisImpl::ProcessCommand(std::shared_ptr<Command> command) {
  bool multi = false;
  for (size_t i = 0; i < (*command->args.args).size(); ++i) {
    const auto& args = (*command->args.args)[i];

    size_t argc = args.size();
    assert(argc >= 1);
    LOG_TRACE() << "command=" << args[0];

    if (IsMultiCommand(args)) multi = true;
    auto is_special = IsSubscribesCommand(args);
    if (is_special) {
      subscriber_ = true;
      reply_privdatum_->subscriber_ = true;
    }

    if ((subscriber_ && !is_special) || !context_) {
      InvokeCommand(command,
                    std::make_shared<Reply>(args[0], nullptr, REDIS_ERR_OTHER));
      continue;
    }

    std::vector<const char*> argv;
    std::vector<size_t> argv_len;

    argv.reserve(argc);
    argv_len.reserve(argc);

    for (size_t i = 0; i < argc; ++i) {
      argv.push_back(args[i].c_str());
      argv_len.push_back(args[i].size());
    }

    {
      if (command->asking && (!multi || IsMultiCommand(args))) {
        static const char* asking = "ASKING";
        static const size_t asking_len = strlen(asking);
        redisAsyncCommandArgv(context_, nullptr, nullptr, 1, &asking,
                              &asking_len);
      }
      redisAsyncCommandArgv(context_, ReplyPrivdatum::OnRedisReply,
                            reinterpret_cast<void*>(cmd_counter_), argc,
                            argv.data(), argv_len.data());
    }

    if (IsExecCommand(args)) multi = false;

    if (!IsUnsubscribeCommand(args)) {
      auto entry = std::make_unique<SingleCommand>();
      entry->cmd = args[0];
      entry->meta = command;
      entry->timer.data = this;
      entry->reply_privdatum = reply_privdatum_;
      ev_timer_init(&entry->timer, OnCommandTimeout,
                    ToEvDuration(command->control.timeout_single), 0.0);
      TimerStart(entry->timer);

      auto& reply_privdata_rev = reply_privdatum_->reply_privdata_rev;
      auto& reply_privdata = reply_privdatum_->reply_privdata;

      assert(!reply_privdata_rev.count(&entry->timer));
      reply_privdata_rev[&entry->timer] = cmd_counter_;
      auto __attribute__((unused)) cmd_iterator =
          reply_privdata.emplace(cmd_counter_, std::move(entry));
      assert(cmd_iterator.second);
    }

    if (!subscriber_) ++sent_count_;
    ++cmd_counter_;
  }
}

}  // namespace redis
}  // namespace storages
