#pragma once

#include <atomic>
#include <cstring>
#include <memory>
#include <vector>

#include <hiredis/async.h>
#include <boost/signals2/signal.hpp>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool.hpp>

#include <storages/redis/impl/base.hpp>
#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/redis_state.hpp>
#include <storages/redis/impl/request.hpp>

namespace redis {

class Statistics;

class Redis {
 public:
  using State = RedisState;
  static const std::string& StateToString(State state);

  Redis(const std::shared_ptr<engine::ev::ThreadPool>& thread_pool,
        bool read_only = false);
  ~Redis();

  Redis(Redis&& o) = delete;

  void Connect(const ConnectionInfo& conn);
  void Connect(const std::string& host, int port, const std::string& password);

  bool AsyncCommand(const CommandPtr& command);
  size_t GetRunningCommands() const;
  std::chrono::milliseconds GetPingLatency() const;
  bool IsDestroying() const;
  std::string GetServerHost() const;

  State GetState() const;
  const Statistics& GetStatistics() const;
  ServerId GetServerId() const;

  boost::signals2::signal<void(State)> signal_state_change;

 private:
  class RedisImpl;
  engine::ev::ThreadControl thread_control_;
  std::shared_ptr<RedisImpl> impl_;
};

template <typename Rep, typename Period>
double ToEvDuration(const std::chrono::duration<Rep, Period>& duration) {
  return std::chrono::duration_cast<std::chrono::duration<double>>(duration)
      .count();
}

}  // namespace redis
