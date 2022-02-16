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
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace io = engine::io;
using Deadline = engine::Deadline;
using TcpListener = io::util_test::TcpListener;

// Certifiactes for testing was generated via the following command:
//   `openssl req -x509 -sha256 -nodes -newkey rsa:2048 -keyout
//    testing_priv.key -out testing_cert.crt`

constexpr auto key = R"(-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDIDeI/2HlQWTpu
Q05Ui8EcPRECcsEuH3j6tndMCXc0omh+Tp33bxYLkDsztzpV4U4LpXPNGVucOYvL
8vaORcB1JTrNdm1Tfu7lQ1YbBkWH7ckhSa9YDa5aLrnB51ckaWL6wSmVnvmxJCXf
CU3RoqRcxHrWq1PV3zHEGXvJB3gOrUKXJKZy/PPhQbCdP8salfxRxmn4BJ8Bxw0k
hydBZcPGT+j3kssBO39OnNWokQ1+E+aa0c9ozAXn6PRPo1M8TSZta8Vwj0Ez8ToR
6eKBbWZIpc8HEwjmllRu1EmljhODf0rBQ4c2TTK7+vXKgjq6VPJ+4Cgtbtw6esqY
vSUzW+KtAgMBAAECggEADu1KRm1GkAI4Y+XNwG01GYBM9pvNYj2i70ISILBMHrdH
iLUhAEdfo7k9UZyIU8Qd6KyNuW388ekxTyRv1SnLNHJ0ssP1YFeGR8EAeb+8DGpn
qX4wASf0LHJ0Fc3HKMZcRk2HZsyX4OnLkZrGHbA/B1T5vW7HMJwYwIMOQ2+1O5L8
MXFxtxmgv9NbOxPnRgSFu0oxpU1PKkq8AwbvnNuMzxhijPVcNPqUu7d2wRDdgca7
KgjCRptlwNaUs44433MHROpH6w1OgSWru/D9+iEHgEoNYu5czErMdDt0RJ/v+WvB
kT1nkegNfo8pDbFE32sjUg2GVJyWW/WRn9NkqYfD2QKBgQDiV3r1RlSXINd3qMPu
jETA+kYHwgjwz8D6jLO+k0wGphoDU1jRntZcksH8haQEgBwYNFSr0UqbdEeFDm1a
4wyaO6orti/CAb7HoM9LbcGhaf28LadeBxVRN4SE6gp+3ghGFq/hbY6csP1ToemS
p+X7xBCb4CJJinqWudKDqENRYwKBgQDiRJpYQLC/GMd1PdJUBtd5i6XRiXyLxNLA
hJ2OzY4vvgp2sLDbNkYNxww+W2GKRGvEukFxGwe7RTq+nOH5uhunhmFVmdnQUUjp
go0j2CWO+67KqDkcIMFduwiLPwLDlkwJvqEB2sXyW/85aROnTZchmIdIV53gdB06
L0aI/o/ArwKBgFAlNc6/9pgE8wbV5XsEhBvpAv8gP9Y1WlndlI/4zETWcAOZcavY
GINzG+l00N0fF9OiRBEK7OYayHBe6W3zU7URR3Ju8n456/n4AS7uUE/9nfESIV90
FqJJjE0cKlc9+6QFyIWEK6lkKm+At5pMhW0ewdrQBJQRytKwPdCMtjmNAoGBANWr
cxVkAYR0IebVOome7FwbQ7tb2gEjHOIwWZlBA8SR8c+ji193ITBhh25bXQD1G8/r
E2F75REzjwXxoHPzC6pnfAMaBlZybCgW4LG4q78abTVzJnspc3DP7oGQP0vz4lpR
ajoWHleACMRuNeEHpHBVWWs3Uh9jhzYq7rDvs1CBAoGBAN8HMss61z7R1EARp/x+
a61QZwEWyK9vLb4DDWDMAQ/1Txx46572pQJ3451pT+exB9zjgq4kJjRY50s4EOjP
FyXMtmRZ5l9WUYXkl6BiElTUhf8ESMNc4xuKFmqv78LkA+PTD6wS6JVEop7KI92Z
9xmxt+dsHh9iH257MzN7dDKd
-----END PRIVATE KEY-----)";

constexpr auto cert = R"(-----BEGIN CERTIFICATE-----
MIIDazCCAlOgAwIBAgIUTwkJtrwdcguh3DGjZHg/aNSk2towDQYJKoZIhvcNAQEL
BQAwRTELMAkGA1UEBhMCUlUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM
GEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDAeFw0yMjAyMTYxNTM0MjVaFw0yMjAz
MTgxNTM0MjVaMEUxCzAJBgNVBAYTAlJVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEw
HwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQDIDeI/2HlQWTpuQ05Ui8EcPRECcsEuH3j6tndMCXc0
omh+Tp33bxYLkDsztzpV4U4LpXPNGVucOYvL8vaORcB1JTrNdm1Tfu7lQ1YbBkWH
7ckhSa9YDa5aLrnB51ckaWL6wSmVnvmxJCXfCU3RoqRcxHrWq1PV3zHEGXvJB3gO
rUKXJKZy/PPhQbCdP8salfxRxmn4BJ8Bxw0khydBZcPGT+j3kssBO39OnNWokQ1+
E+aa0c9ozAXn6PRPo1M8TSZta8Vwj0Ez8ToR6eKBbWZIpc8HEwjmllRu1EmljhOD
f0rBQ4c2TTK7+vXKgjq6VPJ+4Cgtbtw6esqYvSUzW+KtAgMBAAGjUzBRMB0GA1Ud
DgQWBBSuZmgbwSOGEL11DVRQXJ5qr3cA7zAfBgNVHSMEGDAWgBSuZmgbwSOGEL11
DVRQXJ5qr3cA7zAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQBt
UzDe3EOcoNCLJBylztNWnO2mUPwYJ43xtimgqV1D3rMjk1HydjqgdDBJsPxndcnI
sI8idWWzK1sZX/LkZD9xnaeHldjpigP69xpTNreWW1E24DPC3Q5qmN69uDD27XrQ
eXh/s9kIS0PWYR4Sb/xrIxtup1sq4eQreduuFM42jmO4hCToiATiZ7gDpmzWu3rb
4u/fyIfJLB5ZCK3kQ3nhq7oyIOj8QUw88in951wdYTDn6QhS0JKmtYCIueFNqyAW
4O8GaMxzFo7gtzniPTa4P3Oz7XDAOEDFCWhED9apTaMuKbKOvUGMKwDtYSo/asFV
p9lGL6Kd7HsqeEAnWhqu
-----END CERTIFICATE-----)";

constexpr auto other_key = R"(-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDCIpmy2M8h4ewV
4VtymZ4LgE7TVdPHAer8FwsnN3f38H0Y25yQMcMsXxtB6Mwt2gWBcgO1HjCitgrM
xr7Bpgaqsr4QBfM12yA4P505LaqOad8JmO8ZCWaEsCiUoCrPpriVhAcuyMMB1frZ
bAXE+7ngkV1BOIL8aMs7ZLrX5VCMp16aWuI1+PBOkWrz60lfW0sE9cfOXqUtISwI
U+AuSwwNdnn27sJjdwPXh6abk7D/zDDnQspD15q4lZ3oZ9JE8yk+8ZnH6s6Ub/zn
FyxccsEsgnch+PHdRpVGTwIJeTbhQUaDLp9jMTHs0Jl4ept5WlHqg6GhZJi6cNWD
eo9NC8UrAgMBAAECggEATJFI/Xj1eO+aJacUYkakRvT8Ie4f8MWY4IRWKcl+z5NS
Q5OaVGTMDZwVLBGdNhhfQ4asX6rgk8woxks8wTOyPXDx9W/jVMJwGr88S2jvc+vd
w/NFmiJTBNJVrwjc2blv48iG7LTR9M9oeIhaXi7KSQAMP8UCJHtQbMR1zODsvAw2
9LHPZrHboaztgqYbIwJq41WUD8P6ZqBjfeBsN8xgoC6oHqjVNkEBmnPA2HENnggH
LtrAL9kesZvsB/VXP4euFt25xFV63H7afQgpoGLG+DuVhJPfugmSYWB3Aabqwq+7
zV78Gy/nNKOw24ilLQQpTyfATgeq4YbKQj5eqSzVQQKBgQDq0RMHJC3kD8aSfo+P
Dt3aKkkFxyeWqW2cRuOM6blyyjRtI0Utm8/9B44rxelbcxmSuSKWaAbxDjOWc9Ys
2NOnWLsOyuU2+oo55pVktj6JbaneFkMsxhb7PBsrtuw1keUT2aoHGNi/aXvD5TxL
onDe7u8djv/Y2I0mCD0A11lD1QKBgQDTpgPBJ86rY2Rn89tQXp4a/Q6dr9JAZ7Rp
gOeg6dd6PNjn2hIhMFYNbwUsbqT3MV21U732vqUFJukQh1VbsB14kHIek/eTdiWN
zGzL7B9i0XC/E2G9qdUTE+t+gw/GL80KM/0XwP74Xu9cwN3Mu6SIlJFF0S9/DvTD
Hu2uZRpk/wKBgDwcg5t7ZogQhcvwvD1qF98GniTtg7Ps8ZNlDlF/b6r0GpGpysbP
MWJb8chA5Ok1QOGvpSwNu2EwOoKUasqWQzB+5Xv28tCtOH90COB8SuwRd7/TwSSQ
HUf5bhc1v6hDDfqT6RPiB3KQxU8zusi979kSH4JYniRb415OE5fIiSB1AoGAU5Wq
weuQqQr1qkAaCuFzG0F5NjkrlZffHhN0Zo4zNOk22Em9AzJtqZyAtI11xNHQKj5K
NVoRHbnCLW7k/PLOkMCq8PyKt8ffCOMEzHwR4RrJpgxne1nI9mHVjP8Bicly9maV
u33StA/6A+1/Ks2oKvBRdvsoAMvNSgm64Da5d80CgYEAw+UDEDfhJeQV18mnAx8F
QdP6Rsn9S11tcxgkZcipBVERtIDcHjWzQ0iJXoc5Wg9oWrm9f9tanGS8J7AajVab
Egc2oaKznhfXxNWOmZsKcCEtpo4CZUd22byxwOxb8KDP1VNiUNjn0P2iK1rZPIEx
iZgOZHFLfdzjTSk3wfhVG4U=
-----END PRIVATE KEY-----)";

constexpr auto other_cert = R"(-----BEGIN CERTIFICATE-----
MIIDazCCAlOgAwIBAgIUAP3S4EvaXQ1GYgiQZ7ckmDWMrvAwDQYJKoZIhvcNAQEL
BQAwRTELMAkGA1UEBhMCUlUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM
GEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDAeFw0yMjAyMTYxNTM2MzBaFw0yMjAz
MTgxNTM2MzBaMEUxCzAJBgNVBAYTAlJVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEw
HwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQDCIpmy2M8h4ewV4VtymZ4LgE7TVdPHAer8FwsnN3f3
8H0Y25yQMcMsXxtB6Mwt2gWBcgO1HjCitgrMxr7Bpgaqsr4QBfM12yA4P505LaqO
ad8JmO8ZCWaEsCiUoCrPpriVhAcuyMMB1frZbAXE+7ngkV1BOIL8aMs7ZLrX5VCM
p16aWuI1+PBOkWrz60lfW0sE9cfOXqUtISwIU+AuSwwNdnn27sJjdwPXh6abk7D/
zDDnQspD15q4lZ3oZ9JE8yk+8ZnH6s6Ub/znFyxccsEsgnch+PHdRpVGTwIJeTbh
QUaDLp9jMTHs0Jl4ept5WlHqg6GhZJi6cNWDeo9NC8UrAgMBAAGjUzBRMB0GA1Ud
DgQWBBTnBehpbRTQnjhmWK/Ofiu2LhDolTAfBgNVHSMEGDAWgBTnBehpbRTQnjhm
WK/Ofiu2LhDolTAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQCO
uSMK+r/h8f3uB5voR1a7+oou+ClZMJJw2LqVa3G8oGfgHMtIVvgeIzL19HsiB6Ij
3GitIheoGRdfdCS/J6hfDg8+bPqnnWJX9Y5KnHIyAljsDlC2fsTl63MX4zA0mk8o
GVhq2UIAWZiCuVk/+UuIB4F4AH2pwK9LHLx6sHiLeErWDg/+gpB/Da0xmS4gcUy/
KCeuW/gfYHinWfWfVK5HhEhVspDGQ6K7rOu9EB4RyjqN4Z4UysXJYcofk5tKyHVZ
ZG+l3F/qplkTa/s0DsXuaZiYVZ/GusHbhfHxPaSiZrj3qdNZwiABa82iCPftvJ2t
SJd1+pxlLX/rDXAzHLuK
-----END CERTIFICATE-----)";

constexpr auto kShortTimeout = std::chrono::milliseconds{10};

}  // namespace

UTEST_MT(TlsWrapper, Smoke, 2) {
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

  auto server_task = engine::AsyncNoSpan(
      [test_deadline](auto&& server) {
        try {
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
         } catch (const std::exception& e) {
            LOG_ERROR() << e;
            FAIL() << e.what();
         }
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
