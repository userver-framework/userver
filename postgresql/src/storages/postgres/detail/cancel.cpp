#include <storages/postgres/detail/cancel.hpp>

#include <array>
#include <cstdint>

#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/storages/postgres/exceptions.hpp>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PG_PROTOCOL_MAJOR(v) ((v) >> 16)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PG_PROTOCOL_MINOR(v) ((v)&0x0000ffff)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PG_PROTOCOL(m, n) (((m) << 16) | (n))

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CANCEL_REQUEST_CODE PG_PROTOCOL(1234, 5678)

using ACCEPT_TYPE_ARG3 = socklen_t;

struct pg_sockaddr_storage {
  union {
    struct sockaddr sa; /* get the system-dependent fields */
    int64_t ss_align;   /* ensures struct is properly aligned */
    char ss_pad[128];   /* ensures struct has desired size */
  } ss_stuff;
};

// NOLINTNEXTLINE(modernize-use-using)
typedef struct {
  struct pg_sockaddr_storage addr;
  ACCEPT_TYPE_ARG3 salen;
} SockAddr;

struct pg_cancel {
  SockAddr raddr;
  int be_pid;
  int be_key;
};

// NOLINTNEXTLINE(modernize-use-using)
typedef struct CancelRequestPacket {
  /* Note that each field is stored in network byte order! */
  uint32_t cancelRequestCode; /* code to identify a cancel request */
  uint32_t backendPID;        /* PID of client's backend */
  uint32_t cancelAuthCode;    /* secret key to authorize cancel */
} CancelRequestPacket;

struct CancelPacket {
  uint32_t packetlen;
  CancelRequestPacket cp;
};

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

void Cancel(PGcancel* cn, engine::Deadline deadline) {
  if (!cn) return;

  engine::io::Sockaddr addr;
  memcpy(addr.Data(), &cn->raddr,
         std::min<size_t>(sizeof(cn->raddr), addr.Size()));

  engine::io::Socket tmp_sock(addr.Domain(), engine::io::SocketType::kStream);

  tmp_sock.Connect(addr, deadline);

  CancelPacket cp{};
  cp.packetlen = sizeof(cp);
  cp.cp.cancelRequestCode = static_cast<uint32_t>(htonl(CANCEL_REQUEST_CODE));
  cp.cp.backendPID = htonl(cn->be_pid);
  cp.cp.cancelAuthCode = htonl(cn->be_key);

  auto ret = tmp_sock.SendAll(&cp, sizeof(cp), deadline);
  if (ret != sizeof(cp)) throw CommandError("SendAll()");

  /*
   * Comment from libpq's sources, fe-connect.c, inside internal_cancel():
   *
   * Wait for the postmaster to close the connection, which indicates that
   * it's processed the request.  Without this delay, we might issue another
   * command only to find that our cancel zaps that command instead of the
   * one we thought we were canceling.  Note we don't actually expect this
   * read to obtain any data, we are just waiting for EOF to be signaled.
   */
  std::array<char, 1024> c{};
  [[maybe_unused]] auto ret2 = tmp_sock.RecvAll(c.data(), c.size(), deadline);
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
