#pragma once
#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include <hiredis/hiredis.h>

#include <userver/logging/log.hpp>

#include <storages/redis/impl/redis.hpp>
#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/reply.hpp>

#include <boost/asio.hpp>

USERVER_NAMESPACE_BEGIN

namespace io = boost::asio;

class MockRedisServerBase {
 public:
  explicit MockRedisServerBase(int port = 0);
  virtual ~MockRedisServerBase();

  void SendReplyOk(const std::string& reply);
  void SendReplyError(const std::string& reply);
  void SendReplyData(const redis::ReplyData& reply_data);
  int GetPort() const;

 protected:
  void Stop();

  virtual void OnConnected() {}

  virtual void OnDisconnected() {}

  virtual void OnCommand(std::shared_ptr<redis::Reply> cmd) {
    LOG_DEBUG() << "Got command: " << cmd->data.ToDebugString();
  }

  void SendReply(const std::string& reply);
  void Accept();

 private:
  static std::string ReplyDataToRedisProto(const redis::ReplyData& reply_data);
  void Work();
  void OnAccept(boost::system::error_code ec);
  void OnRead(boost::system::error_code ec, size_t count);
  void DoRead();

  io::io_service io_service_;
  io::ip::tcp::acceptor acceptor_;
  std::thread thread_;

  io::ip::tcp::socket socket_;
  std::array<char, 1024> data_{};
  std::unique_ptr<redisReader, decltype(&redisReaderFree)> reader_;
};

class MockRedisServer : public MockRedisServerBase {
 public:
  explicit MockRedisServer(std::string description = {})
      : MockRedisServerBase(0), description_(std::move(description)) {}
  ~MockRedisServer() override;

  using HandlerFunc = std::function<void(const std::vector<std::string>&)>;

  class Handler;
  using HandlerPtr = std::shared_ptr<Handler>;

  HandlerPtr RegisterHandlerWithConstReply(const std::string& command,
                                           redis::ReplyData reply_data);
  HandlerPtr RegisterHandlerWithConstReply(
      const std::string& command, const std::vector<std::string>& args_prefix,
      redis::ReplyData reply_data);
  HandlerPtr RegisterStatusReplyHandler(const std::string& command,
                                        std::string reply);
  HandlerPtr RegisterStatusReplyHandler(
      const std::string& command, const std::vector<std::string>& args_prefix,
      std::string reply);
  HandlerPtr RegisterErrorReplyHandler(const std::string& command,
                                       std::string reply);
  HandlerPtr RegisterErrorReplyHandler(
      const std::string& command, const std::vector<std::string>& args_prefix,
      std::string reply);
  HandlerPtr RegisterNilReplyHandler(const std::string& command);
  HandlerPtr RegisterNilReplyHandler(
      const std::string& command, const std::vector<std::string>& args_prefix);
  HandlerPtr RegisterPingHandler();

  template <typename Rep, typename Period>
  HandlerPtr RegisterTimeoutHandler(
      const std::string& command,
      const std::chrono::duration<Rep, Period>& duration);
  template <typename Rep, typename Period>
  HandlerPtr RegisterTimeoutHandler(
      const std::string& command, const std::vector<std::string>& args_prefix,
      const std::chrono::duration<Rep, Period>& duration);

  struct MasterInfo;
  struct SlaveInfo;

  HandlerPtr RegisterSentinelMastersHandler(
      const std::vector<MasterInfo>& masters);
  HandlerPtr RegisterSentinelSlavesHandler(
      const std::string& master_name, const std::vector<SlaveInfo>& slaves);

  template <typename Rep, typename Period>
  bool WaitForFirstPingReply(
      const std::chrono::duration<Rep, Period>& duration) const;

 protected:
  void OnCommand(std::shared_ptr<redis::Reply> cmd) override;

 private:
  struct CommonMasterSlaveInfo;

  struct HandlerNode {
    std::map<std::string, HandlerNode> next_arg;
    HandlerFunc handler;
  };

  void RegisterHandlerFunc(std::string cmd,
                           const std::vector<std::string>& args_prefix,
                           HandlerFunc handler);

  static void AddHandlerFunc(HandlerNode& root,
                             const std::vector<std::string>& args,
                             HandlerFunc handler);

  static const HandlerFunc& GetHandlerFunc(const HandlerNode& node,
                                           const std::vector<std::string>& args,
                                           size_t arg_idx = 0);

  HandlerPtr DoRegisterTimeoutHandler(
      const std::string& command, const std::vector<std::string>& args_prefix,
      const std::chrono::milliseconds& duration);

  std::string description_;
  std::mutex mutex_;
  std::unordered_map<std::string, HandlerNode> handlers_;
  HandlerPtr ping_handler_;
};

template <typename Rep, typename Period>
MockRedisServer::HandlerPtr MockRedisServer::RegisterTimeoutHandler(
    const std::string& command,
    const std::chrono::duration<Rep, Period>& duration) {
  return DoRegisterTimeoutHandler(
      command, {},
      std::chrono::duration_cast<std::chrono::milliseconds>(duration));
}

template <typename Rep, typename Period>
MockRedisServer::HandlerPtr MockRedisServer::RegisterTimeoutHandler(
    const std::string& command, const std::vector<std::string>& args_prefix,
    const std::chrono::duration<Rep, Period>& duration) {
  return DoRegisterTimeoutHandler(
      command, args_prefix,
      std::chrono::duration_cast<std::chrono::milliseconds>(duration));
}

class MockRedisServer::Handler {
 public:
  template <typename Rep, typename Period>
  bool WaitForFirstReply(const std::chrono::duration<Rep, Period>& duration);

  size_t GetReplyCount() const;

  friend class MockRedisServer;

 private:
  void AccountReply();

  mutable std::mutex mutex_;
  std::condition_variable cv_;
  size_t reply_count_{0};
};

struct MockRedisServer::CommonMasterSlaveInfo {
  std::string name;
  std::string ip;
  int port;
  std::vector<std::string> flags;

  CommonMasterSlaveInfo(std::string name, std::string ip, int port,
                        std::vector<std::string> flags);
};

struct MockRedisServer::MasterInfo
    : public MockRedisServer::CommonMasterSlaveInfo {
  MasterInfo(std::string name, std::string ip, int port,
             std::vector<std::string> flags = {});
};

struct MockRedisServer::SlaveInfo
    : public MockRedisServer::CommonMasterSlaveInfo {
  std::string master_link_status;

  SlaveInfo(std::string name, std::string ip, int port,
            std::vector<std::string> flags = {},
            std::string master_link_status = {"ok"});
};

template <typename Rep, typename Period>
bool MockRedisServer::Handler::WaitForFirstReply(
    const std::chrono::duration<Rep, Period>& duration) {
  std::unique_lock<std::mutex> lock(mutex_);
  return cv_.wait_for(lock, duration, [&] { return reply_count_; });
}

template <typename Rep, typename Period>
bool MockRedisServer::WaitForFirstPingReply(
    const std::chrono::duration<Rep, Period>& duration) const {
  if (!ping_handler_)
    throw std::runtime_error("Ping handler not found for server " +
                             description_);
  return ping_handler_->WaitForFirstReply(duration);
}

USERVER_NAMESPACE_END
