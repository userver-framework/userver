#pragma once

#include <functional>
#include <string>

#include <engine/ev/impl_in_ev_loop.hpp>
#include <engine/ev/thread_control.hpp>

#include "base.hpp"
#include "command.hpp"

namespace storages {
namespace redis {

class RedisImpl;

class Redis : public engine::ev::ImplInEvLoop<RedisImpl> {
 public:
  enum class State {
    kInit = 0,
    kInitError,
    kConnected,
    kConnectHiredisError,
    kConnectError,
    kDisconnected,
    kExitReady
  };

  Redis(const engine::ev::ThreadControl& thread_control,
        bool read_only = false);
  ~Redis();

  Redis(Redis&& o) = delete;
  Redis(const Redis& o) = delete;

  Redis& operator=(Redis&& o) = delete;
  Redis& operator=(const Redis& o) = delete;

  void Connect(const ConnectionInfo& conn,
               std::function<void(State state)> callback);
  void Connect(const std::string& host, int port, const std::string& password,
               std::function<void(State state)> callback);

  bool AsyncCommand(CommandPtr command);
  size_t GetRunningCommands() const;
  bool IsDestroying() const;

  State GetState();
  Stat GetStat();
};

}  // namespace redis
}  // namespace storages
