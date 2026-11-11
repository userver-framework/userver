#include <ugrpc/impl/event_engine/nonblocking_socket.hpp>

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <array>
#include <climits>
#include <csignal>
#include <cstddef>
#include <vector>

#include <sys/socket.h>
#include <sys/uio.h>

#include <gtest/gtest.h>

#include <userver/utils/assert.hpp>

#include <utils/ignore_signal_scope.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {
namespace {

const utils::IgnoreSignalScope ignore_sigpipe_like_components_run{SIGPIPE};

std::array<NonblockingSocket, 2> Pair() {
    std::array<int, 2> fds{};
    UINVARIANT(
        ::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, fds.data()) == 0,
        "socketpair failed"
    );
    return {NonblockingSocket{fds[0]}, NonblockingSocket{fds[1]}};
}

}  // namespace

TEST(GrpcNonblockingSocket, SkipsLeadingEmptyBuffersBeforeApplyingIovecLimit) {
    auto [sender, receiver] = Pair();
    char sent = 'x';
    char received = 0;
    std::vector<iovec> outgoing(static_cast<std::size_t>(IOV_MAX) + 1);
    outgoing.push_back({&sent, 1});
    std::vector<iovec> incoming(static_cast<std::size_t>(IOV_MAX) + 1);
    incoming.push_back({&received, 1});
    ASSERT_EQ(sender.Send(outgoing), 1);
    ASSERT_EQ(receiver.Recv(incoming), 1);
    EXPECT_EQ(received, sent);
}

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
