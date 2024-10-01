#include <storages/postgres/tests/test_buffers.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/bytea.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace static_test {

static_assert(tt::kIsByteaCompatible<std::string>);
static_assert(tt::kIsByteaCompatible<std::string_view>);
static_assert(tt::kIsByteaCompatible<std::vector<char>>);
static_assert(tt::kIsByteaCompatible<std::vector<unsigned char>>);
static_assert(!tt::kIsByteaCompatible<std::vector<bool>>);
static_assert(!tt::kIsByteaCompatible<std::vector<int>>);

}  // namespace static_test

namespace {

using namespace std::string_literals;

const pg::UserTypes types;

const std::string kFooBar = "foo\0bar"s;

using TestByteaWrapper = pg::ByteaWrapper<std::vector<unsigned char>>;

TEST(PostgreIO, Bytea) {
  {
    pg::test::Buffer buffer;
    std::string bin_str{kFooBar};
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, pg::Bytea(bin_str)));
    EXPECT_EQ(kFooBar.size(), buffer.size());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
    std::string tgt_str;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, pg::Bytea(tgt_str)));
    EXPECT_EQ(bin_str, tgt_str);
  }
  {
    pg::test::Buffer buffer;
    std::string_view bin_str{kFooBar.data(), kFooBar.size()};
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, pg::Bytea(bin_str)));
    EXPECT_EQ(kFooBar.size(), buffer.size());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
    std::string_view tgt_str;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, pg::Bytea(tgt_str)));
    EXPECT_EQ(bin_str, tgt_str);
  }
  {
    pg::test::Buffer buffer;
    std::vector<char> bin_str{kFooBar.begin(), kFooBar.end()};
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, pg::Bytea(bin_str)));
    EXPECT_EQ(kFooBar.size(), buffer.size());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
    std::vector<char> tgt_str;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, pg::Bytea(tgt_str)));
    EXPECT_EQ(bin_str, tgt_str);
  }
  {
    pg::test::Buffer buffer;
    std::vector<unsigned char> bin_str{kFooBar.begin(), kFooBar.end()};
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, pg::Bytea(bin_str)));
    EXPECT_EQ(kFooBar.size(), buffer.size());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
    std::vector<unsigned char> tgt_str;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, pg::Bytea(tgt_str)));
    EXPECT_EQ(bin_str, tgt_str);
  }
}

UTEST_P(PostgreConnection, ByteaRoundtrip) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", pg::Bytea(kFooBar)));
  std::string tgt_str;
  UEXPECT_NO_THROW(res[0][0].To(pg::Bytea(tgt_str)));
  EXPECT_EQ(kFooBar, tgt_str);
}

UTEST_P(PostgreConnection, ByteaOwningRoundtrip) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};
  const pg::ByteaWrapper<std::string> wrapped{kFooBar};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", wrapped));
  pg::ByteaWrapper<std::string> returned;
  res[0][0].To(returned);
  UEXPECT_NO_THROW(res[0][0].To(returned));
  EXPECT_EQ(wrapped.bytes, returned.bytes);
}

UTEST_P(PostgreConnection, ByteaStored) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(
      res = GetConn()->Execute(
          "select $1", pg::ParameterStore{}.PushBack(pg::Bytea(kFooBar))));
  std::string tgt_str;
  UEXPECT_NO_THROW(res[0][0].To(pg::Bytea(tgt_str)));
  EXPECT_EQ(kFooBar, tgt_str);
}

UTEST_P(PostgreConnection, ByteaStoredMoved) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};
  pg::ParameterStore store{};
  store.PushBack(pg::Bytea(kFooBar));
  pg::ParameterStore store_moved;
  store_moved = std::move(store);
  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", store_moved));
  std::string tgt_str;
  UEXPECT_NO_THROW(res[0][0].To(pg::Bytea(tgt_str)));
  EXPECT_EQ(kFooBar, tgt_str);
}

UTEST_P(PostgreConnection, ByteaRoundTripTypes) {
  CheckConnection(GetConn());

  {
    /// [bytea_simple]
    pg::ResultSet res = GetConn()->Execute("select 'foobar'::bytea");
    std::string s = "foobar"s;

    // reading a binary string
    std::string received;
    res[0][0].To(pg::Bytea(received));
    EXPECT_EQ(received, s);
    /// [bytea_simple]
  }

  {
    /// [bytea_string]
    // sending a binary string
    std::string s = "\0\xff\x0afoobar"s;
    pg::ResultSet res = GetConn()->Execute("select $1", pg::Bytea(s));

    // reading a binary string
    std::string received;
    res[0][0].To(pg::Bytea(received));
    EXPECT_EQ(received, s);
    /// [bytea_string]
  }

  {
    /// [bytea_vector]
    // storing a byte vector:
    std::vector<std::uint8_t> bin_str{1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto res = GetConn()->Execute("select $1", pg::Bytea(bin_str));

    // reading a byte vector
    std::vector<std::uint8_t> received;
    res[0][0].To(pg::Bytea(received));
    EXPECT_EQ(received, bin_str);
    /// [bytea_vector]
  }
}

UTEST_P(PostgreConnection, ByteaWrapperRowType) {
  CheckConnection(GetConn());

  const auto res = GetConn()->Execute("select 'foobar'::bytea");
  const auto parsed = [&res]() {
    if constexpr (tt::kIsRowType<TestByteaWrapper>) {
      return res.AsContainer<std::vector<TestByteaWrapper>>(pg::kRowTag);
    }
    return res.AsContainer<std::vector<TestByteaWrapper>>();
  }();

  EXPECT_EQ(parsed.size(), 1);

  const std::vector<unsigned char> kExpected{'f', 'o', 'o', 'b', 'a', 'r'};
  EXPECT_EQ(parsed.front().bytes, kExpected);
}

}  // namespace

USERVER_NAMESPACE_END
