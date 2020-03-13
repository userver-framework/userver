#include "tcp_socket.hpp"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include <cstring>

#include <logging/log.hpp>

namespace redis {

boost::optional<std::chrono::microseconds> GetSocketPeerRtt(int fd) {
// MAC_COMPAT
#ifdef TCP_INFO
  int optname = TCP_INFO;
  struct tcp_info ti {};
#else
  int optname = TCP_CONNECTION_INFO;
  struct tcp_connection_info ti {};
#endif
  socklen_t tisize = sizeof(ti);
  if (getsockopt(fd, IPPROTO_TCP, optname, &ti, &tisize) == -1) {
    const auto err_value = errno;
    LOG_ERROR() << "getsockopt failed: " << strerror(err_value);
    return boost::none;
  }
// MAC_COMPAT
#ifdef TCP_INFO
  return std::chrono::microseconds(ti.tcpi_rtt);
#else
  return std::chrono::microseconds(ti.tcpi_rttcur);
#endif
}

}  // namespace redis
