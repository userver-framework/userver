#include <userver/utest/utest.hpp>

#include <openssl/opensslv.h>
#include <sys/socket.h>

#include <stdexcept>
#include <string_view>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/net_listener.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace io = engine::io;
using Deadline = engine::Deadline;
using TcpListener = utest::TcpListener;

// Certificates for testing were generated via the following command:
// openssl req -x509 -sha256 -nodes -newkey rsa:2048
//    -days 3650 -subj '/CN=tlswrapper_test'
//    -keyout testing_priv.key -out testing_cert.crt
//
// NOTE: Ubuntu 20.04 requires RSA of at least 2048 length

constexpr auto key = R"(-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC/yvHZDWFPnDJQ
/1NCDvmj2n3osGSv9+Hl1Caguv5IHSwn8XihcGiPHmm6NFEMU1dZjgUVaZ39nIka
adUI+7dP6SKOYygjtJnJpCNDRKdvO3Axz31gO9DUnOdu7unn4q1ydIYM6To/cwWz
shYFxoGEGwJzIKp93aRX0qMbz3HlUMVlo9tgwflmU4Qm1nYktJVLbsGsUcVPMB3J
+HJq7ciGFs70JH2O6OAISPdSy0+kUhv2u8zN8fRMBMRxn56ZrW66Vyj1C57uJxoW
9U8oEx+U27/YHY/BhmtkViIF+2snH6/cYv4mBliSfWeihBgVhG/IgCI48z7leWLv
0/zFmOtzAgMBAAECggEADU9EIU/wZNnuE/jkCj2HzXsoKbG0CxIktxJV6+mOI+sC
WXNEb8+hMe1mYOmohjZyZWCZsba2pBbs3MxjYFA3lHAVWdQ/wNqToY6mc9Cb3fg9
/PbtOHRuNZL97JDf4pu0dbDobJTy2dxdlO7S4Gu6KTTGor6tljZ/ZSjU8OUgfk1R
wPma33f7z4M03ZzcM0u3i5D+4oD39hFMrNQITk8lv8u0Ha+C8VP6Sov8UKOQ4G2m
kyUs3SGLlmHOIkaIZ2+AfhLvk5b/WgreprObKYJww0PSzFjaIBIdfMBVNieI8aqW
1Qn2FDTras+afjJTTL9exrlP40XJqjvCQtd2O6UtYQKBgQDzo5eDUjGjBo/amOTg
gcgj2yH/sUs0j4Rs5rJ2oqFT45HrGP2JfhMUSlw4uyDFBbaoMzpYOr9S/a+zu+rs
fAZboXdkoD6HcsG29tfsVK+3ZWKlgLm4T2fP7PlxH7wyVqYIqjRKLIXxGYzFjOZO
y4SEoCk0KfxbXQZ5hW1YyoMHRQKBgQDJhffdD/qmjqbSQt7LcH5oeQzmtXYaXzvc
OQRmX6ICxDXdst7eZ1riHXyukkTpaODbEIW7mAP7mMejuxJvImLbZTvDEDWfapwP
gUEUonCQKrY6h14S1Z/CCAiZHkStJrxFRmbQ5b6RXcBgdpyTh4qj5BzrGTooNClC
NH3+/kVXVwKBgCToe2NhaDOSIuiykLmR74e/An+BlCr6Ms1shUyDhnz21HwQ5ReX
CbzhJudRMb2nB+yjFguXmrQvyhYoOYZpo2zuIPAVdmN+duoIqt0aVyQpL7Byt6+8
F7Xf6EnCzPezOKPHZPR3mjLT9AdZOOpm2kRdHuDQG3KbvQdbtxzkUMUhAoGBALNb
IHcHObXzUFXiXhgCTv78fZb3+d0O1V/y/w9+HdsIdkiSYfjfU+vbApT8aYizZyyR
T/TeHu1V1JjMbmOq3wEU4FODobX4VF0YVKvgxv4IhZch04A/0KgILl7YqZbR2s5t
EiTp1Onb3tP7vO8wuxuScoprMW+GvRHHVjwUYfKRAoGAST1MbnT/Wc3HIkI6qOiP
iFqpu4U7AJm9ym/Zc61auoSBKhkWRIcf0sIaQbkH3gOFUKKGmvZ8D2fOn1fco2z+
QXz1smGbGr3A2/scVwRYLef1F/XUHPr85vu0REEtLpizBNGD1D4iViAJk54yRGb2
sWkERQknCgvHIs2c40/YZOE=
-----END PRIVATE KEY-----)";

constexpr auto cert = R"(-----BEGIN CERTIFICATE-----
MIIDFTCCAf2gAwIBAgIUPx6k27O6Tf4me1RyBQ/FWhsn364wDQYJKoZIhvcNAQEL
BQAwGjEYMBYGA1UEAwwPdGxzd3JhcHBlcl90ZXN0MB4XDTIyMDIxNzE3NTMwN1oX
DTMyMDIxNTE3NTMwN1owGjEYMBYGA1UEAwwPdGxzd3JhcHBlcl90ZXN0MIIBIjAN
BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAv8rx2Q1hT5wyUP9TQg75o9p96LBk
r/fh5dQmoLr+SB0sJ/F4oXBojx5pujRRDFNXWY4FFWmd/ZyJGmnVCPu3T+kijmMo
I7SZyaQjQ0SnbztwMc99YDvQ1Jznbu7p5+KtcnSGDOk6P3MFs7IWBcaBhBsCcyCq
fd2kV9KjG89x5VDFZaPbYMH5ZlOEJtZ2JLSVS27BrFHFTzAdyfhyau3IhhbO9CR9
jujgCEj3UstPpFIb9rvMzfH0TATEcZ+ema1uulco9Que7icaFvVPKBMflNu/2B2P
wYZrZFYiBftrJx+v3GL+JgZYkn1nooQYFYRvyIAiOPM+5Xli79P8xZjrcwIDAQAB
o1MwUTAdBgNVHQ4EFgQUvl4L7fVG2b4TxPbhBDvCT7BT924wHwYDVR0jBBgwFoAU
vl4L7fVG2b4TxPbhBDvCT7BT924wDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0B
AQsFAAOCAQEABWB1D1XgMdBabNxxmGq7Y2iBxsonHjuVlASc8InSJHr/jFHjHvS9
y2gtBkQ9P28lIJROkrzXiYM5idyxdijOvwQLKTcJcH95N/h+LPRGvy8a2j2c01LZ
xfDPBOI2KLymnwJS+kQJSm6l5BJBaZxtfnLlgzTaDY1WOOSWmpw0YFS6rbLDrpU3
2Ue8SVlHRBxIID/RgIFcuASSCW+1gvIYDIfvZ9AcS29QPl1oDBoPXAzL8rWOChov
VfX6aXy77rZ1efGfLaWZD1yADp+bg033FkqpOf4PtZpixzbZyDmbh6hrvTuX/y4w
VsNtFCX7LLH/W4mSvkvIws1tm8OtphLn3A==
-----END CERTIFICATE-----)";

// Certificates for testing were generated via the following command:
// openssl req -x509 -sha256 -nodes -newkey rsa:2048
//    -days 3650 -subj '/CN=tlswrapper_test_other'
//    -keyout testing_priv.key -out testing_cert.crt
//
// NOTE: Ubuntu 20.04 requires RSA of at least 2048 length
constexpr auto other_key = R"(-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDRXTl/iZajFxZa
uJpE3yykbFE/o2TYnPQX3LclfToXv6RQIAPjGvwrvBgH4m1I9PaYYpnHolnxEYDN
ur0VJutfkypdXKK9ONcVIx76N98z2hBA3u6XLSmorXu8wsHzgSBioEB1bjEYkvo3
tZooStDgvR/ghCuOXJI4K8qjdUd+T4VhWUeP6qS3KlAOxVqwtr2U5aN3gpJGlZQu
kJykUjQkQp1jcoW13EmYFnE2pRj3Ne3+TLDTVeM2smQf5dwL/eaesU6Yc+Mi8Q8G
tMxH5hzT14b/xG3Fc5nGPUK4Kt8Il/+Nry90UGyFVhnbLcel4i1Sh8QXLZZPeCGM
aJSCnB+7AgMBAAECggEBAJlXLkW7ABlzT2wiyNqomonSy69QfQwp6J2RipJqpaG/
Oxl0WWR83zUpDnC35lMJF5OEpB0TS8zEhRIpM1PKrZnSr7SxpH/yoZVZo9agFVpk
3IKmxRj0ew6QAZC/FE7ExHN3674Wdt8IxzsGR2I7acEww6gtJbmfE3kQmdoei752
LGQbRkNM+DTYYk2yZeCXUVkOqA2J/d4OAPnQyjAhkBRcKU71jF1Zc/IZTWBfgn5e
s4MX2T1KTnnawCBDWMW6oJS8gRRmRGwPkEFVRJ4K4V/kgEhn5wT2mokF+hHYMdo+
V41bqiquvl9j0SqhBnD6OAYQi32npCCf4ddRhlUZbAECgYEA79IhYYiX8uc5Ypnq
lNoR9VN+MAEyECnWrwNlf3obyVj68/KGusjFbljSy/rBSS6ShPQCeSRbzPS3j1B3
nbqZZy+ZUbviAbOpw2D4xN+ur7jVOxxhLjEuTP0lnuPJCFflMQtN5vfkhdcDG89h
YSRdm9MFcPXSysDHzoXgG1KXyB0CgYEA330WOM895a9tDKfxtVppznuZ9mJAEDwV
xQqloZjpnbTbeL3l2CMo2tgcZRjVoYtc1GFE8ke0ZGafujZ+qiGAn8dYCsQgFY+Z
PDKBSbGFQNhq6gcDbA33/Zkh3QvsDxUif5lRSSKEmXbYiTuoo9mUDQ8qKsb4T5v6
uMovOi0d77cCgYEAjlnncJJ4xzkS6gE8qhBrOnjN3UbIZanAAfB9LdbYaYLEq0rZ
SEPmVSKqNWPpmTvowrxoP2oih5z23D3CUsCxT/uEAW0JsULo0M1dvNadRTbscwLc
eGO+/PoCe7bv3GD37U2tdxzL69n9wWMuhU/ltJnkj/GKpskZkPAMX4t+Bs0CgYAf
ln+AkhIul6fzJP2t41SXIbM2NtbVNJjjG8kjWQiUCM8Idta4wOdyXx9MTsFLLvZ0
8jabg/UER9kFqdQnWcrjSnqwMt5SDdTbxEuvzc6Gxs/9ufYK3MKTboRxyNCZpSQW
IuZxTtatFjYu12bTmdoqKl2MZEkOf35lhfY848maawKBgACxJXDPWx/Ah4cT0LbN
XFdWxjSVMU2BHlO0rWy8oZwFaOH8cy+dYe1cdMh+nQLMNXXTY+nu4WditpLfURof
2GmF9LD5e6tUJw1rPLbKDVM5DXuO3ka0msRea4Y8Diaiql8eLytJhbMw12nE0B1g
LKaaQekBQ5eebwM3oRc68q0K
-----END PRIVATE KEY-----)";

constexpr auto other_cert = R"(-----BEGIN CERTIFICATE-----
MIIDITCCAgmgAwIBAgIUL5c8L0jabF9rZ2w383zGQmwcLZwwDQYJKoZIhvcNAQEL
BQAwIDEeMBwGA1UEAwwVdGxzd3JhcHBlcl90ZXN0X290aGVyMB4XDTIyMDIxNzEy
MjcxOVoXDTMyMDIxNTEyMjcxOVowIDEeMBwGA1UEAwwVdGxzd3JhcHBlcl90ZXN0
X290aGVyMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA0V05f4mWoxcW
WriaRN8spGxRP6Nk2Jz0F9y3JX06F7+kUCAD4xr8K7wYB+JtSPT2mGKZx6JZ8RGA
zbq9FSbrX5MqXVyivTjXFSMe+jffM9oQQN7uly0pqK17vMLB84EgYqBAdW4xGJL6
N7WaKErQ4L0f4IQrjlySOCvKo3VHfk+FYVlHj+qktypQDsVasLa9lOWjd4KSRpWU
LpCcpFI0JEKdY3KFtdxJmBZxNqUY9zXt/kyw01XjNrJkH+XcC/3mnrFOmHPjIvEP
BrTMR+Yc09eG/8RtxXOZxj1CuCrfCJf/ja8vdFBshVYZ2y3HpeItUofEFy2WT3gh
jGiUgpwfuwIDAQABo1MwUTAdBgNVHQ4EFgQU947UQygfsOw+udnjR3rywSlqnZIw
HwYDVR0jBBgwFoAU947UQygfsOw+udnjR3rywSlqnZIwDwYDVR0TAQH/BAUwAwEB
/zANBgkqhkiG9w0BAQsFAAOCAQEAZUrhuONUlLalvmtjaVEsNgoPErF8jPeLHHGA
9dVAvkaumbwacO5AYWbODRhC0NCdYTNAqLaQVA4utxZGI3Z2pQKxI+OnlCM+ws3z
Nrfh+E/6EJZblfqK7SBduPYlFxhkalQ19mi/R5E6p9EmBdV1sScR6Hsg3qSCpFbT
Z49kghXJ5KAS4jOB6SxClxbR5Tpc1E3khduX6aGau1VOkgPJxfdqHHqsyUc1RH/Z
3W6n4SmhCZxEpQzSQUw4YPIIpWuSUWUr7MS7TzGDdT4AgD0m5VLFbF2ca/Fsz2WW
v2R63aFo/UfQSQ4dhC0o2Vy74DyLwnO3pH8wudfBJ8/LX/Uz/A==
-----END CERTIFICATE-----)";

constexpr auto kShortTimeout = std::chrono::milliseconds{10};

}  // namespace

UTEST_MT(TlsWrapper, Smoke, 2) {
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

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
  const auto deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  /// [TLS wrapper usage]
  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(deadline);

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
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);
  EXPECT_THROW(
      static_cast<void>(io::TlsWrapper::StartTlsClient(
          std::move(client), {}, Deadline::FromDuration(kShortTimeout))),
      io::IoTimeout);
  EXPECT_THROW(static_cast<void>(io::TlsWrapper::StartTlsServer(
                   std::move(server), crypto::Certificate::LoadFromString(cert),
                   crypto::PrivateKey::LoadFromString(key),
                   Deadline::FromDuration(kShortTimeout))),
               io::IoException);
}

UTEST_MT(TlsWrapper, IoTimeout, 2) {
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

  engine::SingleConsumerEvent timeout_happened;
  auto server_task = engine::AsyncNoSpan(
      [test_deadline, &timeout_happened](auto&& server) {
        auto tls_server = io::TlsWrapper::StartTlsServer(
            std::move(server), crypto::Certificate::LoadFromString(cert),
            crypto::PrivateKey::LoadFromString(key), test_deadline);
        char c = 0;
        UEXPECT_THROW(static_cast<void>(tls_server.RecvSome(
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
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

  auto server_task = engine::AsyncNoSpan(
      [test_deadline](auto&& server) {
        auto tls_server = io::TlsWrapper::StartTlsServer(
            std::move(server), crypto::Certificate::LoadFromString(cert),
            crypto::PrivateKey::LoadFromString(key), test_deadline);
        char c = 0;
        UEXPECT_THROW(
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
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

  auto server_task = engine::AsyncNoSpan(
      [test_deadline](auto&& server) {
        UEXPECT_THROW(
            static_cast<void>(io::TlsWrapper::StartTlsServer(
                std::move(server), crypto::Certificate::LoadFromString(cert),
                crypto::PrivateKey::LoadFromString(other_key), test_deadline)),
            io::TlsException);
      },
      std::move(server));

  EXPECT_THROW(static_cast<void>(io::TlsWrapper::StartTlsClient(
                   std::move(client), {}, test_deadline)),
               io::IoException);
  server_task.Get();
}

UTEST_MT(TlsWrapper, NonTlsClient, 2) {
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

  auto server_task = engine::AsyncNoSpan(
      [test_deadline](auto&& server) {
        UEXPECT_THROW(
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
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

  auto server_task = engine::AsyncNoSpan(
      [test_deadline](auto&& server) {
        EXPECT_EQ(5, server.SendAll("hello", 5, test_deadline));
      },
      std::move(server));

  EXPECT_THROW(static_cast<void>(io::TlsWrapper::StartTlsClient(
                   std::move(client), {}, test_deadline)),
               io::IoException);
  server_task.Get();
}

UTEST_MT(TlsWrapper, DoubleSmoke, 4) {
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

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
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  UEXPECT_THROW(
      static_cast<void>(io::TlsWrapper::StartTlsClient({}, {}, test_deadline)),
      io::TlsException);
  UEXPECT_THROW(static_cast<void>(io::TlsWrapper::StartTlsServer(
                    {}, crypto::Certificate::LoadFromString(cert),
                    crypto::PrivateKey::LoadFromString(key), test_deadline)),
                io::TlsException);
}

UTEST(TlsWrapper, PeerShutdown) {
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

  auto server_task = engine::AsyncNoSpan(
      [test_deadline](auto&& server) {
        try {
          auto tls_server = io::TlsWrapper::StartTlsServer(
              std::move(server), crypto::Certificate::LoadFromString(cert),
              crypto::PrivateKey::LoadFromString(key), test_deadline);
          char c = 0;
          // Get a non-fatal error on the channel
          EXPECT_THROW(
              [[maybe_unused]] const auto ret = tls_server.RecvAll(
                  &c, 1,
                  engine::Deadline::FromDuration(std::chrono::milliseconds{1})),
              io::IoTimeout);
          ASSERT_EQ(1, tls_server.SendAll(&c, 1, test_deadline));
          EXPECT_EQ(0, tls_server.RecvSome(&c, 1, test_deadline));
        } catch (const std::exception& e) {
          LOG_ERROR() << e;
          FAIL() << e.what();
        }
      },
      std::move(server));

  {
    auto tls_client =
        io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline);
    char c = 0;
    ASSERT_EQ(1, tls_client.RecvAll(&c, 1, test_deadline));
    // destroy the wrapper causing an unidirectional shutdown
  }

  server_task.Get();
}

UTEST(TlsWrapper, PeerDisconnect) {
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  TcpListener tcp_listener;
  auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

  auto server_task = engine::AsyncNoSpan(
      [test_deadline](auto&& server) {
        try {
          auto tls_server = io::TlsWrapper::StartTlsServer(
              std::move(server), crypto::Certificate::LoadFromString(cert),
              crypto::PrivateKey::LoadFromString(key), test_deadline);
          char c = 0;
          // Get a non-fatal error on the channel
          EXPECT_THROW(
              [[maybe_unused]] const auto ret = tls_server.RecvAll(
                  &c, 1,
                  engine::Deadline::FromDuration(std::chrono::milliseconds{1})),
              io::IoTimeout);
          ASSERT_EQ(1, tls_server.SendAll(&c, 1, test_deadline));
          EXPECT_THROW([[maybe_unused]] const auto ret =
                           tls_server.RecvAll(&c, 1, test_deadline),
                       io::TlsException);
        } catch (const std::exception& e) {
          LOG_ERROR() << e;
          FAIL() << e.what();
        }
      },
      std::move(server));

  {
    const auto client_fd = client.Fd();
    auto tls_client =
        io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline);
    char c = 0;
    ASSERT_EQ(1, tls_client.RecvAll(&c, 1, test_deadline));
    // disconnect the underlying socket without shutting down the channel
    ::shutdown(client_fd, SHUT_RDWR);
  }

  server_task.Get();
}

USERVER_NAMESPACE_END
