#include <userver/net/blocking/get_addr_info.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(GetAddrInfo, Localhost) {
  auto data = net::blocking::GetAddrInfo("localhost", "8080");
  EXPECT_GE(data.size(), 1);
  for (const auto& addr : data) {
    EXPECT_EQ(addr.Port(), 8080);
    EXPECT_TRUE(addr.PrimaryAddressString() == "127.0.0.1" ||
                addr.PrimaryAddressString() == "::1");
  }
}

TEST(GetAddrInfo, LocalhostTelnet) {
  auto data = net::blocking::GetAddrInfo("localhost", "telnet");
  EXPECT_GE(data.size(), 1);
  for (const auto& addr : data) {
    EXPECT_EQ(addr.Port(), 23);
    EXPECT_TRUE(addr.PrimaryAddressString() == "127.0.0.1" ||
                addr.PrimaryAddressString() == "::1");
  }
}

TEST(GetAddrInfo, LocalhostIpV4) {
  auto data = net::blocking::GetAddrInfo("127.0.0.1", "8085");
  EXPECT_GE(data.size(), 1);
  EXPECT_EQ(data.front().Port(), 8085);
  EXPECT_EQ(data.front().PrimaryAddressString(), "127.0.0.1");
}

TEST(GetAddrInfo, LocalhostIpV6) {
  auto data = net::blocking::GetAddrInfo("::1", "8095");
  EXPECT_GE(data.size(), 1);
  EXPECT_EQ(data.front().Port(), 8095);
  EXPECT_EQ(data.front().PrimaryAddressString(), "::1");
}

TEST(GetAddrInfo, NotResoveHost) {
  EXPECT_THROW(net::blocking::GetAddrInfo("unknown_host.local", "8085"),
               std::runtime_error);
}

TEST(GetAddrInfo, NotValidIp) {
  EXPECT_THROW(net::blocking::GetAddrInfo("1277.0.0.1", "8089"),
               std::runtime_error);
}

USERVER_NAMESPACE_END
