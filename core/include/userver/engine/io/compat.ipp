// Temporary compatibility layer for TAXICOMMON-4358

namespace engine::io {

using AddrStorage = Sockaddr;

struct Addr {
  Addr(const Sockaddr& sockaddr, int type, int /*protocol*/)
      : sockaddr{sockaddr}, type{static_cast<SocketType>(type)} {}

  Addr(const void* native, int type, int /*protocol*/)
      : sockaddr{native}, type{static_cast<SocketType>(type)} {}

  Sockaddr sockaddr;
  SocketType type;
};

inline logging::LogHelper& operator<<(logging::LogHelper& lh,
                                      const Addr& addr) {
  return lh << addr.sockaddr;
}

[[nodiscard]] inline Socket Connect(const Addr& addr, Deadline deadline) {
  Socket socket{addr.sockaddr.Domain(), addr.type};
  socket.Connect(addr.sockaddr, deadline);
  return socket;
}

[[nodiscard]] inline Socket Listen(const Addr& addr, int backlog) {
  Socket socket{addr.sockaddr.Domain(), addr.type};
  socket.Bind(addr.sockaddr);
  socket.Listen(backlog);
  return socket;
}

}  // namespace engine::io
