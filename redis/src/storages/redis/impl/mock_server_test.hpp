#pragma once
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include <hiredis/hiredis.h>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>

#include <storages/redis/impl/redis.hpp>
#include <userver/storages/redis/base.hpp>
#include <userver/storages/redis/reply.hpp>

USERVER_NAMESPACE_BEGIN

class MockRedisServerBase {
public:
    struct Connection {
        explicit Connection(engine::io::Socket socket)
            : socket(std::move(socket))
        {}

        engine::io::Socket socket;
        std::array<char, 1024> data{};
        std::unique_ptr<redisReader, decltype(&redisReaderFree)> reader{nullptr, &redisReaderFree};
    };
    using ConnectionPtr = std::shared_ptr<Connection>;

    explicit MockRedisServerBase(int port = 0);
    virtual ~MockRedisServerBase();

    void SendReplyOk(ConnectionPtr connection, const std::string& reply);
    void SendReplyError(ConnectionPtr connection, const std::string& reply);
    void SendReplyData(ConnectionPtr connection, const storages::redis::ReplyData& reply_data);
    int GetPort() const;

protected:
    void Stop();

    virtual void OnConnected(ConnectionPtr /*connection*/) {}

    virtual void OnDisconnected(ConnectionPtr /*connection*/) {}

    virtual void OnCommand(ConnectionPtr /*connection*/, std::shared_ptr<storages::redis::Reply> cmd) {
        LOG_DEBUG() << "Got command: " << cmd->data.ToDebugString();
    }

private:
    static std::string ReplyDataToRedisProto(const storages::redis::ReplyData& reply_data);
    void AcceptLoop();
    void HandleConnection(ConnectionPtr connection);

    void SendReply(ConnectionPtr connection, const std::string& reply);

    std::uint16_t port_{};
    engine::io::Socket listener_;
    concurrent::BackgroundTaskStorage client_tasks_;
    engine::Task listener_task_;
};

class MockRedisServer : public MockRedisServerBase {
public:
    explicit MockRedisServer(std::string description)
        : MockRedisServerBase(0),
          description_(std::move(description))
    {
        UASSERT(!description_.empty());
    }
    ~MockRedisServer() override;

    using HandlerFunc = std::function<void(ConnectionPtr, const std::vector<std::string>&)>;

    class Handler;
    using HandlerPtr = std::shared_ptr<Handler>;

    HandlerPtr RegisterHandlerWithConstReply(const std::string& command, storages::redis::ReplyData reply_data);
    HandlerPtr RegisterHandlerWithConstReply(
        const std::string& command,
        const std::vector<std::string>& args_prefix,
        storages::redis::ReplyData reply_data
    );
    HandlerPtr RegisterStatusReplyHandler(const std::string& command, std::string reply);
    HandlerPtr RegisterStatusReplyHandler(
        const std::string& command,
        const std::vector<std::string>& args_prefix,
        std::string reply
    );
    HandlerPtr RegisterErrorReplyHandler(const std::string& command, std::string reply);
    HandlerPtr RegisterErrorReplyHandler(
        const std::string& command,
        const std::vector<std::string>& args_prefix,
        std::string reply
    );
    HandlerPtr RegisterNilReplyHandler(const std::string& command);
    HandlerPtr RegisterNilReplyHandler(const std::string& command, const std::vector<std::string>& args_prefix);
    HandlerPtr RegisterPingHandler();

    template <typename Rep, typename Period>
    HandlerPtr RegisterTimeoutHandler(const std::string& command, const std::chrono::duration<Rep, Period>& duration);
    template <typename Rep, typename Period>
    HandlerPtr RegisterTimeoutHandler(
        const std::string& command,
        const std::vector<std::string>& args_prefix,
        const std::chrono::duration<Rep, Period>& duration
    );

    struct MasterInfo;
    struct SlaveInfo;

    HandlerPtr RegisterSentinelMastersHandler(const std::vector<MasterInfo>& masters);
    HandlerPtr RegisterSentinelSlavesHandler(const std::string& master_name, const std::vector<SlaveInfo>& slaves);
    HandlerPtr RegisterClusterNodes(const std::vector<MasterInfo>& masters, const std::vector<SlaveInfo>& slaves);

    template <typename Rep, typename Period>
    bool WaitForFirstPingReply(const std::chrono::duration<Rep, Period>& duration) const;

protected:
    void OnCommand(ConnectionPtr connection, std::shared_ptr<storages::redis::Reply> cmd) override;

private:
    struct CommonMasterSlaveInfo;

    struct HandlerNode {
        std::map<std::string, HandlerNode> next_arg;
        HandlerFunc handler;
    };

    void RegisterHandlerFunc(std::string cmd, const std::vector<std::string>& args_prefix, HandlerFunc handler);

    static void AddHandlerFunc(HandlerNode& root, const std::vector<std::string>& args, HandlerFunc handler);

    static const HandlerFunc& GetHandlerFunc(
        const HandlerNode& node,
        const std::vector<std::string>& args,
        size_t arg_idx = 0
    );

    HandlerPtr DoRegisterTimeoutHandler(
        const std::string& command,
        const std::vector<std::string>& args_prefix,
        const std::chrono::milliseconds& duration
    );

    const std::string description_;
    engine::Mutex mutex_;
    std::unordered_map<std::string, HandlerNode> handlers_;
    HandlerPtr ping_handler_;
};

template <typename Rep, typename Period>
MockRedisServer::HandlerPtr MockRedisServer::RegisterTimeoutHandler(
    const std::string& command,
    const std::chrono::duration<Rep, Period>& duration
) {
    return DoRegisterTimeoutHandler(command, {}, std::chrono::duration_cast<std::chrono::milliseconds>(duration));
}

template <typename Rep, typename Period>
MockRedisServer::HandlerPtr MockRedisServer::RegisterTimeoutHandler(
    const std::string& command,
    const std::vector<std::string>& args_prefix,
    const std::chrono::duration<Rep, Period>& duration
) {
    return DoRegisterTimeoutHandler(
        command,
        args_prefix,
        std::chrono::duration_cast<std::chrono::milliseconds>(duration)
    );
}

class MockRedisServer::Handler {
public:
    template <typename Rep, typename Period>
    bool WaitForFirstReply(const std::chrono::duration<Rep, Period>& duration);

    size_t GetReplyCount() const;

    friend class MockRedisServer;

private:
    void AccountReply();

    mutable engine::Mutex mutex_;
    engine::ConditionVariable cv_;
    size_t reply_count_{0};
};

struct MockRedisServer::CommonMasterSlaveInfo {
    std::string name;
    std::string ip;
    int port;
    std::vector<std::string> flags;

    CommonMasterSlaveInfo(std::string name, std::string ip, int port, std::vector<std::string> flags);
};

struct MockRedisServer::MasterInfo : public MockRedisServer::CommonMasterSlaveInfo {
    MasterInfo(std::string name, std::string ip, int port, std::vector<std::string> flags = {});
};

struct MockRedisServer::SlaveInfo : public MockRedisServer::CommonMasterSlaveInfo {
    std::string master_link_status;

    SlaveInfo(
        std::string name,
        std::string ip,
        int port,
        std::vector<std::string> flags = {},
        std::string master_link_status = {"ok"}
    );
};

template <typename Rep, typename Period>
bool MockRedisServer::Handler::WaitForFirstReply(const std::chrono::duration<Rep, Period>& duration) {
    std::unique_lock lock{mutex_};
    return cv_.WaitFor(lock, duration, [&] { return reply_count_; });
}

template <typename Rep, typename Period>
bool MockRedisServer::WaitForFirstPingReply(const std::chrono::duration<Rep, Period>& duration) const {
    if (!ping_handler_) {
        throw std::runtime_error("Ping handler not found for server " + description_);
    }
    return ping_handler_->WaitForFirstReply(duration);
}

USERVER_NAMESPACE_END
