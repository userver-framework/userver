#include <storages/postgres/detail/cancel.hpp>

// Internal structures of libpq to send cancel packet using async non blocking socket

#include <array>
#include <cstdint>

#include <sys/socket.h>

#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/tracing/span.hpp>

#include <storages/postgres/detail/pg_version.hpp>

// NOLINTNEXTLINE(modernize-use-using)
typedef struct {
    struct sockaddr_storage addr;
    socklen_t salen;
} SockAddr;

#if USERVER_LIBPQ_VERSION >= 180000

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"
#endif

struct pg_cancel {
    SockAddr raddr;          /* Remote address */
    int be_pid;              /* PID of to-be-canceled backend */
    int pgtcp_user_timeout;  /* tcp user timeout */
    int keepalives;          /* use TCP keepalives? */
    int keepalives_idle;     /* time between TCP keepalives */
    int keepalives_interval; /* time between TCP keepalive
                              * retransmits */
    int keepalives_count;    /* maximum number of TCP keepalive
                              * retransmits */

    /* Pre-constructed cancel request packet starts here */
    int32_t cancel_pkt_len;                     /* in network byte order */
    char cancel_req[/*FLEXIBLE_ARRAY_MEMBER*/]; /* CancelRequestPacket */
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#else

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PG_PROTOCOL_MAJOR(v) ((v) >> 16)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PG_PROTOCOL_MINOR(v) ((v) & 0x0000ffff)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PG_PROTOCOL(m, n) (((m) << 16) | (n))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CANCEL_REQUEST_CODE PG_PROTOCOL(1234, 5678)

// NOLINTNEXTLINE(modernize-use-using)
typedef struct CancelRequestPacket {
    /* Note that each field is stored in network byte order! */
    uint32_t cancelRequestCode; /* code to identify a cancel request */
    uint32_t backendPID;        /* PID of client's backend */
    uint32_t cancelAuthCode;    /* secret key to authorize cancel */
} CancelRequestPacket;

struct pg_cancel {
    SockAddr raddr;
    int be_pid;
    int be_key;
};

#endif

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

namespace {

void SendCancelPacket(engine::io::Socket& tmp_sock, engine::Deadline deadline, const PGcancel& cn) {
#if USERVER_LIBPQ_VERSION >= 180000
    // reference code: libpq/fe-cancel.c
    // In libpq cancel packet is stored directly in pg_cancel struct
    // starting from cancel_pkt_len field
    // After this field pg_cancel contains flexible array with binary prepared packet
    // NOTE: cancel_pkt_len in pg_cancel stored in network byte order
    size_t cancel_pkt_len = ntohl(cn.cancel_pkt_len);
    auto ret = tmp_sock.SendAll(reinterpret_cast<const char*>(&cn.cancel_pkt_len), cancel_pkt_len, deadline);
    if (ret != cancel_pkt_len) {
        throw CommandError(fmt::format(
            "SendCancelPacket: Failed to call SendAll(), expected bytes to send {}, actual bytes sent {}",
            cancel_pkt_len,
            ret
        ));
    }
#else
    struct {
        uint32_t packetlen;
        CancelRequestPacket cp;
    } crp;

    crp.packetlen = htonl(sizeof(crp));
    crp.cp.cancelRequestCode = htonl(CANCEL_REQUEST_CODE);
    crp.cp.backendPID = htonl(cn.be_pid);
    crp.cp.cancelAuthCode = htonl(cn.be_key);

    auto ret = tmp_sock.SendAll(&crp, sizeof(crp), deadline);
    if (ret != sizeof(crp)) {
        throw CommandError("SendAll()");
    }
#endif
}

}  // namespace

void Cancel(PGcancel* cn, engine::Deadline deadline) {
    tracing::Span span{"pg_cancel"};
    if (!cn) {
        return;
    }

    engine::io::Sockaddr addr(&cn->raddr.addr);
    engine::io::Socket tmp_sock(addr.Domain(), engine::io::SocketType::kStream);

    tmp_sock.Connect(addr, deadline);
    LOG_DEBUG() << "Connected to " << addr.PrimaryAddressString() << " on port " << addr.Port();

    SendCancelPacket(tmp_sock, deadline, *cn);

    /*
     * Comment from libpq's sources, fe-connect.c, inside internal_cancel():
     *
     * Wait for the postmaster to close the connection, which indicates that
     * it's processed the request.  Without this delay, we might issue another
     * command only to find that our cancel zaps that command instead of the
     * one we thought we were canceling.  Note we don't actually expect this
     * read to obtain any data, we are just waiting for EOF to be signaled.
     */
    try {
        std::array<char, 1024> c{};
        [[maybe_unused]] auto ret = tmp_sock.RecvAll(c.data(), c.size(), deadline);
    } catch (const engine::io::IoSystemError& ex) {
        LOG_LIMITED_INFO() << "Got exception during RecvAll after pg_cancel: " << ex;
        // Do not propagate exception here, because postmaster sends no response
        // for cancel packets, just silently close socket
    }
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
