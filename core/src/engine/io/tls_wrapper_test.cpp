#include <userver/utest/utest.hpp>

#include <openssl/opensslv.h>

#include <stdexcept>
#include <string_view>
#include <vector>

#include <engine/io/util_test.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace io = engine::io;
using Deadline = engine::Deadline;
using TcpListener = io::util_test::TcpListener;

constexpr auto key = R"(-----BEGIN PRIVATE KEY-----
MIICdgIBADANBgkqhkiG9w0BAQEFAASCAmAwggJcAgEAAoGBANOkJFJ4M1bG+jjJ
xvQNU1rhQwAF3Bs8kkUM5ic85Cgep9u0Hepm0ZNbGF0ggJRC3OjPKiD6Y0WqZMo5
8kosoMxcG/Ad+3UEsPSielYCQ6S338F6MYD69Ku/iaij4Kg+p8rnxVX0q820OlKN
EjIshsGM36Ulp2i1H578izO0cORdAgMBAAECgYAZx6HBBfFC/cPXDQUWD1V6+Xt+
0jfofW1XyeVzHCCynmFWCs+mENYwe+Uw2teut6JPHYUFNVrITqQuWfaggGUpprrY
OkYLTUMcgt8cCAODQ2nzuTsfJHhwpGSIW6Fy6i481nPQV/c8miXGpE4+rx1Q+OnY
OAx5VWoXzTZbPsCt4QJBAPTslBrb8/uZdaRf0h9ZQyohzoTNNFURbKrnV4fC084y
Tz0drTxBE1CDZGCiHYo+tm8gtjpoi2IIgS/Vm/LUBFUCQQDdNkFkUBnUlT4WU8KL
KN981c2oThBv5ff1dU0YmGB18AwD6WfB4/WjnUAjHA/49iuwp+dzUg+koDuc/KQh
kCfpAkAlUNor0XE11yauWY8JCa+K/sWZRC6B+3qj+0VBwPRGSTH7bMcVFBEeRjaH
5os7odxnyAMbmQwLbqJIKHJvJ9BVAkAWPYczi64dJmgYnJE5poFZrrE/k6GpbmiQ
oBuBNoi0Ms8ycXwCDWY77ept3Ttp324jE658dKqn9Ygoz2m9Ch5JAkEA3LdSNqG3
7yWFW0JhggartFs6fiY/LhDEghoNtj/CZa5O6h7vxImS5PwxRTRPmzboDSJWAb73
TlLf+ZalVZlwNw==
-----END PRIVATE KEY-----)";

constexpr auto cert = R"(-----BEGIN CERTIFICATE-----
MIICEDCCAXmgAwIBAgIUeY4SOP67qSccdbwgg/P5hTTA0NAwDQYJKoZIhvcNAQEL
BQAwGjEYMBYGA1UEAwwPdGxzd3JhcHBlcl90ZXN0MB4XDTIyMDEyMTAxNTUwNloX
DTMyMDExOTAxNTUwNlowGjEYMBYGA1UEAwwPdGxzd3JhcHBlcl90ZXN0MIGfMA0G
CSqGSIb3DQEBAQUAA4GNADCBiQKBgQDTpCRSeDNWxvo4ycb0DVNa4UMABdwbPJJF
DOYnPOQoHqfbtB3qZtGTWxhdIICUQtzozyog+mNFqmTKOfJKLKDMXBvwHft1BLD0
onpWAkOkt9/BejGA+vSrv4moo+CoPqfK58VV9KvNtDpSjRIyLIbBjN+lJadotR+e
/IsztHDkXQIDAQABo1MwUTAdBgNVHQ4EFgQUDbslb0/VyGXUXOR70i7T7sdzYvMw
HwYDVR0jBBgwFoAUDbslb0/VyGXUXOR70i7T7sdzYvMwDwYDVR0TAQH/BAUwAwEB
/zANBgkqhkiG9w0BAQsFAAOBgQDEA8FVKlP0gG/1rCbfrUNxz0sFmDo99yk14AL6
GrXLklZh3Hl3rlZCGKN1OrKPRS+d2Rke9Egn2FexY6lhOrTYG1OLVF1llgOp6jqR
+yOXWnBrIvtfWQpo5SaEyNIduhOfRSZI5dr1MMkPUmOecQ5rldzFXX65HynxomZm
xsQt0A==
-----END CERTIFICATE-----)";

constexpr auto other_key = R"(-----BEGIN PRIVATE KEY-----
MIICdgIBADANBgkqhkiG9w0BAQEFAASCAmAwggJcAgEAAoGBANSaWt86DA/pVU/K
hmhNGUnLtExC/PqiiGyUTGdpMI6DdxPRQx3HuZLBFiXuxpQSoWb2fvkxLVZA9UWs
SdvdSakU5uBl3lWMwqjwdaw+Sf865ZWy+VsQhF3FiXxw50L4Dyf5rtCv95696vxy
nY3DQWApKUsBwMYAgYnsOv1vjEVhAgMBAAECgYBtVhR1yLx7Ulx1dIo7Cat/sCtB
LRI9M2lFrd69L/Ow1xteLbh+kEB3oKVrTKkDbbFj6lDSht/yA+YftsMlN2Cxi/rK
3PcwyHIE6caYkGs2odNpkWyYZftqIX68D/rVjuCb8DgjA0kO+T0GqDerDshR2ZEp
I0CCpHBZgQoHfBocoQJBAOyeOFHrBeFsNa9enA8YPk204jy+uo7tlavd+iWZqcyh
G+ObNdojFMr9i0lDG0SlIwB9WH1FefpCtc2kYhclK98CQQDmBIxumF049T9VLSnl
J5z5/vhU8bIrRlQvek71RO1/W/qUb3S1xsn6IMnl+nm8vxzg8mnTODqrQi+uRN/2
YLa/AkBxeB6CCjazt3S3OKOWCYY3NXsYrk5ApGaWGMkQpvPqkYgSSig1B4W9IoFd
DLVS4e47GeEJkfvAq6ULjL0NZGH5AkBBzr9WzOSu7QuHlPpNg33X0Gi/9L5ivyZK
xxVb+rJwI6KXYSPk9dDHbSYWVAkMRSk/+lrogUfXw4Hcu/vPg3AFAkEApmgSmiLi
cZUtUEPrJPcRGcmG8PWcb/MYWSUJ0VeAKksPra+gAV/rZoqSPoHDfJsPX6uQ49s7
KQ/rNWbmvEFh8w==
-----END PRIVATE KEY-----)";

constexpr auto other_cert = R"(-----BEGIN CERTIFICATE-----
MIICHDCCAYWgAwIBAgIUKg7kzga/MptAM24v7ZMUSsVvodowDQYJKoZIhvcNAQEL
BQAwIDEeMBwGA1UEAwwVdGxzd3JhcHBlcl90ZXN0X290aGVyMB4XDTIyMDEyNjIy
MTUzM1oXDTMyMDEyNDIyMTUzM1owIDEeMBwGA1UEAwwVdGxzd3JhcHBlcl90ZXN0
X290aGVyMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDUmlrfOgwP6VVPyoZo
TRlJy7RMQvz6oohslExnaTCOg3cT0UMdx7mSwRYl7saUEqFm9n75MS1WQPVFrEnb
3UmpFObgZd5VjMKo8HWsPkn/OuWVsvlbEIRdxYl8cOdC+A8n+a7Qr/eever8cp2N
w0FgKSlLAcDGAIGJ7Dr9b4xFYQIDAQABo1MwUTAdBgNVHQ4EFgQUtq4Y0dNUPYxj
/HbnnbXUPjphFlwwHwYDVR0jBBgwFoAUtq4Y0dNUPYxj/HbnnbXUPjphFlwwDwYD
VR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOBgQDSRlimIT009w8F+Rk4K0GH
k+rLtoW+wSMPOC2WkWxX4zlLxhpIxvjkHjMw55WjNeMu7Ps9J9wXx89q3as1fMe0
/B/iv0vPyuqKr5JDrEb+TYnJZqH2KrwFyAOTu0GD1ejr2NM/IYjns/XQV2JnTvIR
IxL3KlTyfphZYAAZq+2HDw==
-----END CERTIFICATE-----)";

constexpr auto kShortTimeout = std::chrono::milliseconds{10};

}  // namespace

UTEST_MT(TlsWrapper, Smoke, 2) {
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

  auto server_task = engine::AsyncNoSpan(
      [test_deadline](auto&& server) {
        auto tls_server = io::TlsWrapper::StartTlsServer(
            std::move(server), crypto::Certificate::LoadFromString(cert),
            crypto::PrivateKey::LoadFromString(key), test_deadline);
        EXPECT_EQ(1, tls_server.SendAll("1", 1, test_deadline));
        char c = 0;
        EXPECT_EQ(1, tls_server.RecvSome(&c, 1, test_deadline));
        EXPECT_EQ('2', c);

        auto raw_server = tls_server.StopTls(test_deadline);
        EXPECT_EQ(1, raw_server.SendAll("3", 1, test_deadline));
        EXPECT_EQ(1, raw_server.RecvSome(&c, 1, test_deadline));
        EXPECT_EQ('4', c);
      },
      std::move(server));

  auto tls_client =
      io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline);
  char c = 0;
  EXPECT_EQ(1, tls_client.RecvSome(&c, 1, test_deadline));
  EXPECT_EQ('1', c);
  EXPECT_EQ(1, tls_client.SendAll("2", 1, test_deadline));
  auto raw_client = tls_client.StopTls(test_deadline);
  EXPECT_EQ(1, raw_client.RecvSome(&c, 1, test_deadline));
  EXPECT_EQ('3', c);
  EXPECT_EQ(1, raw_client.SendAll("4", 1, test_deadline));

  server_task.Get();
}

UTEST_MT(TlsWrapper, DocTest, 2) {
  static constexpr std::string_view kData = "hello world";
  const auto deadline = Deadline::FromDuration(kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(deadline);

  /// [TLS wrapper usage]
  auto server_task = utils::Async(
      "tls-server",
      [deadline](auto&& server) {
        auto tls_server = io::TlsWrapper::StartTlsServer(
            std::move(server), crypto::Certificate::LoadFromString(cert),
            crypto::PrivateKey::LoadFromString(key), deadline);
        if (tls_server.SendAll(kData.data(), kData.size(), deadline) !=
            kData.size()) {
          throw std::runtime_error("Couldn't send data");
        }
      },
      std::move(server));

  auto tls_client =
      io::TlsWrapper::StartTlsClient(std::move(client), {}, deadline);
  std::vector<char> buffer(kData.size());
  const auto bytes_rcvd =
      tls_client.RecvAll(buffer.data(), buffer.size(), deadline);
  /// [TLS wrapper usage]

  server_task.Get();
  std::string_view result(buffer.data(), bytes_rcvd);
  EXPECT_EQ(result, kData.substr(0, result.size()));
}

UTEST(TlsWrapper, ConnectTimeout) {
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);
  EXPECT_THROW(
      static_cast<void>(io::TlsWrapper::StartTlsClient(
          std::move(client), {}, Deadline::FromDuration(kShortTimeout))),
      io::TlsException);
  EXPECT_THROW(static_cast<void>(io::TlsWrapper::StartTlsServer(
                   std::move(server), crypto::Certificate::LoadFromString(cert),
                   crypto::PrivateKey::LoadFromString(key),
                   Deadline::FromDuration(kShortTimeout))),
               io::TlsException);
}

UTEST_MT(TlsWrapper, IoTimeout, 2) {
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

  engine::SingleConsumerEvent timeout_happened;
  auto server_task = engine::AsyncNoSpan(
      [test_deadline, &timeout_happened](auto&& server) {
        auto tls_server = io::TlsWrapper::StartTlsServer(
            std::move(server), crypto::Certificate::LoadFromString(cert),
            crypto::PrivateKey::LoadFromString(key), test_deadline);
        char c = 0;
        EXPECT_THROW(static_cast<void>(tls_server.RecvSome(
                         &c, 1, Deadline::FromDuration(kShortTimeout))),
                     io::IoTimeout);
        timeout_happened.Send();
    // OpenSSL 1.0 always breaks the channel here. Please update.
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
        EXPECT_EQ(1, tls_server.RecvSome(&c, 1, test_deadline));
        EXPECT_EQ('1', c);
#endif
      },
      std::move(server));

  auto tls_client =
      io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline);
  ASSERT_TRUE(timeout_happened.WaitForEventUntil(test_deadline));
  // see above
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
  EXPECT_EQ(1, tls_client.SendAll("1", 1, test_deadline));
#endif
  server_task.Get();
}

UTEST(TlsWrapper, Cancel) {
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

  auto server_task = engine::AsyncNoSpan(
      [test_deadline](auto&& server) {
        auto tls_server = io::TlsWrapper::StartTlsServer(
            std::move(server), crypto::Certificate::LoadFromString(cert),
            crypto::PrivateKey::LoadFromString(key), test_deadline);
        char c = 0;
        EXPECT_THROW(
            static_cast<void>(tls_server.RecvSome(&c, 1, test_deadline)),
            io::IoInterrupted);
      },
      std::move(server));

  auto tls_client =
      io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline);

  engine::Yield();
  server_task.SyncCancel();
}

UTEST_MT(TlsWrapper, CertKeyMismatch, 2) {
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

  auto server_task = engine::AsyncNoSpan(
      [test_deadline](auto&& server) {
        EXPECT_THROW(
            static_cast<void>(io::TlsWrapper::StartTlsServer(
                std::move(server), crypto::Certificate::LoadFromString(cert),
                crypto::PrivateKey::LoadFromString(other_key), test_deadline)),
            io::TlsException);
      },
      std::move(server));

  EXPECT_THROW(static_cast<void>(io::TlsWrapper::StartTlsClient(
                   std::move(client), {}, test_deadline)),
               io::TlsException);
  server_task.Get();
}

UTEST_MT(TlsWrapper, NonTlsClient, 2) {
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

  auto server_task = engine::AsyncNoSpan(
      [test_deadline](auto&& server) {
        EXPECT_THROW(
            static_cast<void>(io::TlsWrapper::StartTlsServer(
                std::move(server), crypto::Certificate::LoadFromString(cert),
                crypto::PrivateKey::LoadFromString(other_key), test_deadline)),
            io::TlsException);
      },
      std::move(server));

  EXPECT_EQ(5, client.SendAll("hello", 5, test_deadline));
  server_task.Get();
}

UTEST_MT(TlsWrapper, NonTlsServer, 2) {
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

  auto server_task = engine::AsyncNoSpan(
      [test_deadline](auto&& server) {
        EXPECT_EQ(5, server.SendAll("hello", 5, test_deadline));
      },
      std::move(server));

  EXPECT_THROW(static_cast<void>(io::TlsWrapper::StartTlsClient(
                   std::move(client), {}, test_deadline)),
               io::TlsException);
  server_task.Get();
}

UTEST_MT(TlsWrapper, DoubleSmoke, 4) {
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);
  auto [other_server, other_client] =
      tcp_listener.MakeSocketPair(test_deadline);

  auto server_task = engine::AsyncNoSpan(
      [test_deadline](auto&& server) {
        auto tls_server = io::TlsWrapper::StartTlsServer(
            std::move(server), crypto::Certificate::LoadFromString(cert),
            crypto::PrivateKey::LoadFromString(key), test_deadline);
        EXPECT_EQ(1, tls_server.SendAll("1", 1, test_deadline));
        char c = 0;
        EXPECT_EQ(1, tls_server.RecvSome(&c, 1, test_deadline));
        EXPECT_EQ('2', c);

        auto raw_server = tls_server.StopTls(test_deadline);
        EXPECT_EQ(1, raw_server.SendAll("3", 1, test_deadline));
        EXPECT_EQ(1, raw_server.RecvSome(&c, 1, test_deadline));
        EXPECT_EQ('4', c);
      },
      std::move(server));

  auto other_server_task = engine::AsyncNoSpan(
      [test_deadline](auto&& server) {
        auto tls_server = io::TlsWrapper::StartTlsServer(
            std::move(server), crypto::Certificate::LoadFromString(other_cert),
            crypto::PrivateKey::LoadFromString(other_key), test_deadline);
        EXPECT_EQ(1, tls_server.SendAll("5", 1, test_deadline));
        char c = 0;
        EXPECT_EQ(1, tls_server.RecvSome(&c, 1, test_deadline));
        EXPECT_EQ('6', c);

        auto raw_server = tls_server.StopTls(test_deadline);
        EXPECT_EQ(1, raw_server.SendAll("7", 1, test_deadline));
        EXPECT_EQ(1, raw_server.RecvSome(&c, 1, test_deadline));
        EXPECT_EQ('8', c);
      },
      std::move(other_server));

  auto other_client_task = engine::AsyncNoSpan(
      [test_deadline](auto&& client) {
        auto tls_client = io::TlsWrapper::StartTlsClient(std::move(client), {},
                                                         test_deadline);
        char c = 0;
        EXPECT_EQ(1, tls_client.RecvSome(&c, 1, test_deadline));
        EXPECT_EQ('5', c);
        EXPECT_EQ(1, tls_client.SendAll("6", 1, test_deadline));
        auto raw_client = tls_client.StopTls(test_deadline);
        EXPECT_EQ(1, raw_client.RecvSome(&c, 1, test_deadline));
        EXPECT_EQ('7', c);
        EXPECT_EQ(1, raw_client.SendAll("8", 1, test_deadline));
      },
      std::move(other_client));

  auto tls_client =
      io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline);
  char c = 0;
  EXPECT_EQ(1, tls_client.RecvSome(&c, 1, test_deadline));
  EXPECT_EQ('1', c);
  EXPECT_EQ(1, tls_client.SendAll("2", 1, test_deadline));
  auto raw_client = tls_client.StopTls(test_deadline);
  EXPECT_EQ(1, raw_client.RecvSome(&c, 1, test_deadline));
  EXPECT_EQ('3', c);
  EXPECT_EQ(1, raw_client.SendAll("4", 1, test_deadline));

  server_task.Get();
  other_server_task.Get();
  other_client_task.Get();
}

UTEST(TlsWrapper, InvalidSocket) {
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  EXPECT_THROW(
      static_cast<void>(io::TlsWrapper::StartTlsClient({}, {}, test_deadline)),
      io::TlsException);
  EXPECT_THROW(static_cast<void>(io::TlsWrapper::StartTlsServer(
                   {}, crypto::Certificate::LoadFromString(cert),
                   crypto::PrivateKey::LoadFromString(key), test_deadline)),
               io::TlsException);
}

USERVER_NAMESPACE_END
