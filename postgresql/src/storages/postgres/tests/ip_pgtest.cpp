#include <storages/postgres/tests/test_buffers.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>

#include <userver/storages/postgres/io/ip.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace {

using namespace std::string_literals;
const pg::UserTypes types;

template <typename T>
void IpAddressReadWriteTest(const T& address) {
  pg::test::Buffer buffer;
  UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, address));
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
    T tgt_address;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt_address));
    EXPECT_EQ(address, tgt_address);
}

template <typename T>
void IpAddressRoundtrip(storages::postgres::detail::ConnectionPtr& conn, const T& address) {
  
  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = conn->Execute("select $1", address));
  T tgt_address;
  UEXPECT_NO_THROW(res[0][0].To(tgt_address));
  EXPECT_EQ(address, tgt_address);
}


TEST(PostgreIO, IpAddress) {
  using namespace utils::ip;
  IpAddressReadWriteTest(AddressV4FromString("127.0.0.1"));
  IpAddressReadWriteTest(AddressV6FromString("ffff::"));
  const auto netv4 = NetworkV4FromString("192.168.1.0/24");
  IpAddressReadWriteTest(netv4);
  const auto netv6 = NetworkV6FromString("ffff::/120");
  IpAddressReadWriteTest(netv6);
  IpAddressReadWriteTest(NetworkV4ToInetNetwork(netv4));
  IpAddressReadWriteTest(NetworkV6ToInetNetwork(netv6));
}

UTEST_P(PostgreConnection, IpAddressRoundtrip) {
  using namespace utils::ip;
  CheckConnection(GetConn());
  IpAddressRoundtrip(GetConn(), AddressV4FromString("127.0.0.1"));
  IpAddressRoundtrip(GetConn(), AddressV6FromString("ffff::"));
  const auto netv4 = NetworkV4FromString("192.168.1.0/24");
  IpAddressRoundtrip(GetConn(), netv4);
  const auto netv6 = NetworkV6FromString("ffff::/120");
  IpAddressRoundtrip(GetConn(), netv6);
  IpAddressRoundtrip(GetConn(), NetworkV4ToInetNetwork(netv4));
  IpAddressRoundtrip(GetConn(), NetworkV6ToInetNetwork(netv6));
}

}  // namespace

USERVER_NAMESPACE_END
