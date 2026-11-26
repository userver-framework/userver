#pragma once

/// @file userver/utest/port_reservation.hpp
/// @brief @copybrief utest::PortReservation

#include <chrono>
#include <cstdint>
#include <optional>

#include <userver/fs/blocking/file_descriptor.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

/// Socket settings for a port reservation.
struct PortReservationOptions {
    /// Address family of the reserved socket.
    enum class IpVersion : std::uint8_t {
        kV4,  ///< IPv4.
        kV6,  ///< IPv6.
    };

    /// Transport protocol of the reserved socket.
    enum class Protocol : std::uint8_t {
        kTcp,  ///< TCP.
        kUdp,  ///< UDP.
    };

    /// Address family.
    IpVersion ip_version{IpVersion::kV4};

    /// Transport protocol.
    Protocol protocol{Protocol::kTcp};

    /// Nonzero port to reserve. Otherwise the OS assigns a port.
    /// Parallel test runs in CI are likely to contend for the same fixed port.
    /// Use this option only as a last resort when the subsystem under test does not support configuring its port.
    std::optional<std::uint16_t> fixed_port{};

    /// How long to wait for a fixed port.
    std::chrono::milliseconds timeout{std::chrono::seconds{30}};
};

/// @ingroup userver_utest
/// @brief Owns a socket bound to a port on an IPv4 or IPv6 loopback address.
/// Keep the reservation alive until the consumer has duplicated the descriptor.
class PortReservation final {
public:
    using Options = PortReservationOptions;
    using IpVersion = Options::IpVersion;
    using Protocol = Options::Protocol;

    /// Binds the socket without enabling port sharing.
    /// For a fixed port, this can block the calling thread until the port becomes available.
    /// @throws std::system_error If binding fails or the fixed port remains busy until the timeout.
    /// @throws std::invalid_argument If the fixed port is zero.
    explicit PortReservation(Options options = {});

    PortReservation(const PortReservation&) = delete;
    PortReservation(PortReservation&&) noexcept = default;
    PortReservation& operator=(const PortReservation&) = delete;
    PortReservation& operator=(PortReservation&&) noexcept = default;

    /// Returns whether the reservation still owns its socket.
    bool IsValid() const noexcept;

    /// Returns a borrowed descriptor. Requires a valid reservation. The caller must not close it.
    int GetNativeFd() const;

    /// Transfers ownership of the bound descriptor to the caller and invalidates this reservation.
    /// Requires a valid reservation.
    [[nodiscard]] int ReleaseNativeFd() &&;

    /// Returns the assigned port. Requires a reservation that still owns its socket.
    std::uint16_t GetPort() const;

private:
    fs::blocking::FileDescriptor fd_;
};

}  // namespace utest

USERVER_NAMESPACE_END
