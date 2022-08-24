#pragma once

#include <atomic>
#include <cstring>
#include <memory>
#include <vector>

#include <boost/signals2/signal.hpp>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool.hpp>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/command.hpp>
#include <userver/storages/redis/impl/redis_state.hpp>
#include <userver/storages/redis/impl/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

class Statistics;

class Redis {
 public:
  using State = RedisState;
  static const std::string& StateToString(State state);

  Redis(const std::shared_ptr<engine::ev::ThreadPool>& thread_pool,
        bool send_readonly = false);
  ~Redis();

  Redis(Redis&& o) = delete;

  void Connect(const std::string& host, int port, const Password& password);

  bool AsyncCommand(const CommandPtr& command);
  size_t GetRunningCommands() const;
  std::chrono::milliseconds GetPingLatency() const;
  bool IsDestroying() const;
  std::string GetServerHost() const;

  State GetState() const;
  const Statistics& GetStatistics() const;
  ServerId GetServerId() const;

  void SetCommandsBufferingSettings(
      CommandsBufferingSettings commands_buffering_settings);

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  boost::signals2::signal<void(State)> signal_state_change;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  boost::signals2::signal<void()> signal_not_in_cluster_mode;

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

USERVER_NAMESPACE_END
