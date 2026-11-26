#include <userver/utest/port_reservation.hpp>

#include <netinet/in.h>
#include <sys/socket.h>

#include <chrono>
#include <future>
#include <string>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

namespace {

static_assert(!std::is_copy_constructible_v<PortReservation>);
static_assert(!std::is_copy_assignable_v<PortReservation>);
static_assert(std::is_nothrow_move_constructible_v<PortReservation>);
static_assert(std::is_nothrow_move_assignable_v<PortReservation>);

struct ExpectedSocketOptions {
    int family;
    int type;
};

void ExpectBoundSocket(const PortReservation& reservation, ExpectedSocketOptions expected) {
    EXPECT_NE(reservation.GetPort(), 0);
    sockaddr_storage address{};
    socklen_t size = sizeof(address);
    ASSERT_EQ(::getsockname(reservation.GetNativeFd(), reinterpret_cast<sockaddr*>(&address), &size), 0);
    ASSERT_EQ(address.ss_family, expected.family);
    if (expected.family == AF_INET) {
        EXPECT_EQ(reinterpret_cast<const sockaddr_in*>(&address)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    } else {
        const auto& ipv6 = reinterpret_cast<const sockaddr_in6*>(&address)->sin6_addr;
        EXPECT_TRUE(IN6_IS_ADDR_LOOPBACK(&ipv6));
    }

    int socket_type{};
    size = sizeof(socket_type);
    ASSERT_EQ(::getsockopt(reservation.GetNativeFd(), SOL_SOCKET, SO_TYPE, &socket_type, &size), 0);
    EXPECT_EQ(socket_type, expected.type);
}

TEST(PortReservation, DefaultsToIpv4Tcp) {
    const PortReservation reservation;
    ExpectBoundSocket(reservation, {.family = AF_INET, .type = SOCK_STREAM});
}

TEST(PortReservation, MovesIpv6UdpSocket) {
    PortReservation
        reservation({.ip_version = PortReservation::IpVersion::kV6, .protocol = PortReservation::Protocol::kUdp});
    const auto port = reservation.GetPort();
    const auto fd = reservation.GetNativeFd();

    PortReservation moved(std::move(reservation));
    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_FALSE(reservation.IsValid());
    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_UINVARIANT_FAILURE(static_cast<void>(reservation.GetNativeFd()));
    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_UINVARIANT_FAILURE(static_cast<void>(reservation.GetPort()));
    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_UINVARIANT_FAILURE(static_cast<void>(std::move(reservation).ReleaseNativeFd()));

    ASSERT_TRUE(moved.IsValid());
    EXPECT_EQ(moved.GetPort(), port);
    EXPECT_EQ(moved.GetNativeFd(), fd);

    PortReservation assigned;
    assigned = std::move(moved);
    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_FALSE(moved.IsValid());
    ASSERT_TRUE(assigned.IsValid());

    EXPECT_EQ(assigned.GetPort(), port);
    EXPECT_EQ(assigned.GetNativeFd(), fd);
    ExpectBoundSocket(assigned, {.family = AF_INET6, .type = SOCK_DGRAM});
}

TEST(PortReservation, WaitsForFixedPort) {
    PortReservation occupied;
    const auto port = occupied.GetPort();
    auto hold_port = std::async(std::launch::async, [occupied = std::move(occupied)]() mutable {
        const PortReservation held{std::move(occupied)};
        std::this_thread::sleep_for(std::chrono::milliseconds{75});
    });

    const PortReservation reservation({.fixed_port = port});
    EXPECT_EQ(reservation.GetPort(), port);
    hold_port.get();
}

TEST(PortReservation, TimesOutOnBusyFixedPort) {
    const PortReservation occupied;
    const auto port = occupied.GetPort();
    constexpr std::chrono::milliseconds kTimeout{25};
    const auto deadline = std::chrono::steady_clock::now() + kTimeout;

    try {
        const PortReservation reservation({.fixed_port = port, .timeout = kTimeout});
        FAIL() << "Reserved an occupied port";
    } catch (const std::system_error& error) {
        EXPECT_EQ(error.code(), std::errc::timed_out);
        EXPECT_GE(std::chrono::steady_clock::now(), deadline);
        EXPECT_NE(std::string{error.what()}.find(std::to_string(port)), std::string::npos);
    }
}

}  // namespace
}  // namespace utest

USERVER_NAMESPACE_END
