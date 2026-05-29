#pragma once

#include <cstdint>
#include <string>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

class PostgresChaosProxy final {
public:
    PostgresChaosProxy(engine::TaskProcessor& task_processor, std::string postgres_host, std::uint16_t postgres_port);
    ~PostgresChaosProxy();

    std::uint16_t GetPort() const;

private:
    void AcceptorLoop();
    void ProcessSocket(engine::io::Socket proxied_socket, engine::io::Socket db_socket);

    engine::io::Sockaddr postgres_addr_;
    engine::io::Socket proxy_socket_;
    engine::Task acceptor_task_;

    std::uint16_t port_{0};

    engine::TaskProcessor& task_processor_;
    concurrent::BackgroundTaskStorageCore task_storage_;
};

USERVER_NAMESPACE_END
