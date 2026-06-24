#include <gtest/gtest.h>

#include <initializer_list>

#include <storages/postgres/tests/test_buffers.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/ip.hpp>
#include <userver/storages/postgres/io/macaddr.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;
namespace ip = utils::ip;

namespace {

const pg::UserTypes types;

pg::test::Buffer MakeBuffer(std::initializer_list<unsigned char> bytes) {
    pg::test::Buffer buffer;
    for (const auto b : bytes) {
        buffer.push_back(static_cast<char>(b));
    }
    return buffer;
}

// A server-sent inet/cidr field shorter than the fixed 4-byte header must be
// rejected instead of reading past the field buffer.
TEST(PostgreIOIpAddressBuffer, ShortInetHeader) {
    const auto buffer = MakeBuffer({0x02, 0x20});  // 2 bytes, header needs 4
    auto fb = pg::test::MakeFieldBuffer(buffer);
    pg::AddressV4 tgt;
    UEXPECT_THROW(io::ReadBuffer(fb, tgt), pg::InvalidInputBufferSize);
}

// A 4-byte header that declares a 16-byte address but carries no address bytes
// must be rejected before the memcpy (std::array parser path, AddressV6).
TEST(PostgreIOIpAddressBuffer, TruncatedInet6Address) {
    const auto buffer = MakeBuffer({0x03, 0x80, 0x01, 0x10});  // declared size = 16
    auto fb = pg::test::MakeFieldBuffer(buffer);
    pg::AddressV6 tgt;
    UEXPECT_THROW(io::ReadBuffer(fb, tgt), pg::InvalidInputBufferSize);
}

// Same defect through the std::vector parser path (InetNetwork).
TEST(PostgreIOIpAddressBuffer, TruncatedInetNetwork) {
    const auto buffer = MakeBuffer({0x03, 0x80, 0x00, 0x10});  // declared size = 16
    auto fb = pg::test::MakeFieldBuffer(buffer);
    pg::InetNetwork tgt;
    UEXPECT_THROW(io::ReadBuffer(fb, tgt), pg::InvalidInputBufferSize);
}

// A valid address still round-trips after the bounds check.
TEST(PostgreIOIpAddressBuffer, ValidAddressRoundtrip) {
    const auto address = ip::AddressV4FromString("127.0.0.1");
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, address));
    auto fb = pg::test::MakeFieldBuffer(buffer);
    pg::AddressV4 tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
    EXPECT_EQ(address, tgt);
}

// A macaddr field shorter than the fixed octet count must be rejected.
TEST(PostgreIOIpAddressBuffer, ShortMacaddr) {
    const auto buffer = MakeBuffer({0xaa, 0xbb, 0xcc});  // 3 of 6 octets
    auto fb = pg::test::MakeFieldBuffer(buffer);
    pg::Macaddr tgt;
    UEXPECT_THROW(io::ReadBuffer(fb, tgt), pg::InvalidInputBufferSize);
}

TEST(PostgreIOIpAddressBuffer, ShortMacaddr8) {
    const auto buffer = MakeBuffer({0xaa, 0xbb, 0xcc, 0xdd, 0xee});  // 5 of 8 octets
    auto fb = pg::test::MakeFieldBuffer(buffer);
    pg::Macaddr8 tgt;
    UEXPECT_THROW(io::ReadBuffer(fb, tgt), pg::InvalidInputBufferSize);
}

}  // namespace

USERVER_NAMESPACE_END
