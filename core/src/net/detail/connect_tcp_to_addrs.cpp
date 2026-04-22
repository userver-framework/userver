#include <userver/net/detail/connect_tcp_to_addrs.hpp>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <string>
#include <vector>

#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace net::detail {

engine::io::Socket ConnectTcpToAddrs(
    std::string_view host,
    std::uint16_t port,
    const std::vector<engine::io::Sockaddr>& addrs,
    engine::Deadline deadline
) {
    std::vector<std::string> connect_errors;

    for (const auto& current_addr : addrs) {
        try {
            const engine::TaskCancellationBlocker block_cancel;
            engine::io::Socket socket{current_addr.Domain(), engine::io::SocketType::kStream};
            socket.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
            socket.Connect(current_addr, deadline);
            return socket;
        } catch (const engine::io::IoCancelled& ex) {
            throw engine::io::IoException(ex.what());
        } catch (const engine::io::IoException& ex) {
            LOG_DEBUG() << "Cannot connect to " << host << " at " << current_addr << ": " << ex;
            connect_errors.push_back(current_addr.PrimaryAddressString() + ": " + ex.what());
        }
    }

    if (!connect_errors.empty()) {
        std::string message = "Cannot connect to " + std::string{host} + ":" + std::to_string(port);
        for (const auto& err : connect_errors) {
            message += "; " + err;
        }
        throw engine::io::IoException(message);
    }

    throw engine::io::IoException{
        "Cannot connect to " + std::string{host} + ":" + std::to_string(port) + " (no addresses resolved)"
    };
}

}  // namespace net::detail

USERVER_NAMESPACE_END
