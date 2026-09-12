#include <userver/utest/utest.hpp>

#include <bitset>
#include <initializer_list>

#include <storages/postgres/tests/test_buffers.hpp>

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/bitstring.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;

namespace {

const pg::UserTypes types;

pg::test::Buffer MakeBuffer(std::initializer_list<unsigned char> bytes) {
    pg::test::Buffer buffer;
    for (const auto b : bytes) {
        buffer.push_back(static_cast<char>(b));
    }
    return buffer;
}

// A server-sent bit/varbit field whose 4-byte length prefix is negative must be
// rejected instead of being silently accepted as an empty bit string.
TEST(PostgreIOBitStringBuffer, NegativeBitCount) {
    const auto buffer = MakeBuffer({0xFF, 0xFF, 0xFF, 0xFF});  // bit count = -1
    auto fb = pg::test::MakeFieldBuffer(buffer);
    std::bitset<8> tgt;
    UEXPECT_THROW(io::ReadBuffer(fb, tgt), pg::InvalidBitStringRepresentation);
}

// A bit count near INT_MAX used to overflow `bit_count + 7` (signed overflow,
// UB) before the byte-count comparison; the field must be rejected.
TEST(PostgreIOBitStringBuffer, HugeBitCount) {
    const auto buffer = MakeBuffer({0x7F, 0xFF, 0xFF, 0xFF});  // bit count = INT_MAX
    auto fb = pg::test::MakeFieldBuffer(buffer);
    std::bitset<8> tgt;
    UEXPECT_THROW(io::ReadBuffer(fb, tgt), pg::InvalidBitStringRepresentation);
}

// A valid bit string still round-trips unchanged after the added validation.
TEST(PostgreIOBitStringBuffer, ValidRoundtrip) {
    const std::bitset<8> src{0xF2};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    auto fb = pg::test::MakeFieldBuffer(buffer);
    std::bitset<8> tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
    EXPECT_EQ(src, tgt);
}

}  // namespace

USERVER_NAMESPACE_END
