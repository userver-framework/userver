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

}  // namespace

USERVER_NAMESPACE_END
