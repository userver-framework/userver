#include <benchmark/benchmark.h>

#include <openssl/opensslv.h>
#include <sys/socket.h>

#include <stdexcept>
#include <string_view>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/internal/net/net_listener.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace io = engine::io;
using Deadline = engine::Deadline;
using TcpListener = internal::net::TcpListener;

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

constexpr auto kDeadlineMaxTime = std::chrono::seconds{60};

}  // namespace

[[maybe_unused]] void tls_write_all_buffered(benchmark::State& state) {
  engine::RunStandalone(2, [&]() {
    const auto deadline = Deadline::FromDuration(kDeadlineMaxTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(deadline);

    std::atomic<bool> reading{true};
    auto server_task = engine::AsyncNoSpan(
        [&reading, deadline](auto&& server) {
          auto tls_server = io::TlsWrapper::StartTlsServer(
              std::forward<decltype(server)>(server),
              crypto::Certificate::LoadFromString(cert),
              crypto::PrivateKey::LoadFromString(key), deadline);

          std::array<std::byte, 16'384> buf{};
          while (tls_server.RecvSome(buf.data(), buf.size(), deadline) > 0 &&
                 reading) {
            /* receiving msgs */
          }
        },
        std::move(server));

    engine::io::IoData msg{"msg", 3};
    const std::string payload(state.range(0), 'x');
    engine::io::IoData big_msg{payload.data(), payload.size()};

    auto tls_client =
        io::TlsWrapper::StartTlsClient(std::move(client), {}, deadline);

    for ([[maybe_unused]] auto _ : state) {
      auto send_bytes = tls_client.WriteAll(
          {msg, msg, msg, big_msg, msg, msg, big_msg, msg, big_msg, msg, msg},
          deadline);
      benchmark::DoNotOptimize(send_bytes);
    }

    reading.store(false);
    server_task.Get();
  });
}

BENCHMARK(tls_write_all_buffered)
    ->RangeMultiplier(2)
    ->Range(1 << 6, 1 << 12)
    ->Unit(benchmark::kNanosecond);

[[maybe_unused]] void tls_write_all_default(benchmark::State& state) {
  engine::RunStandalone(2, [&]() {
    const auto deadline = Deadline::FromDuration(kDeadlineMaxTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(deadline);

    std::atomic<bool> reading{true};
    auto server_task = engine::AsyncNoSpan(
        [&reading, deadline](auto&& server) {
          auto tls_server = io::TlsWrapper::StartTlsServer(
              std::forward<decltype(server)>(server),
              crypto::Certificate::LoadFromString(cert),
              crypto::PrivateKey::LoadFromString(key), deadline);

          std::array<std::byte, 16'384> buf{};
          while (tls_server.RecvSome(buf.data(), buf.size(), deadline) > 0 &&
                 reading) {
            /* receiving msgs */
          }
        },
        std::move(server));

    engine::io::IoData msg{"msg", 3};
    const std::string payload(state.range(0), 'x');
    engine::io::IoData big_msg{payload.data(), payload.size()};

    auto tls_client =
        io::TlsWrapper::StartTlsClient(std::move(client), {}, deadline);

    std::size_t send_bytes{0};
    for ([[maybe_unused]] auto _ : state) {
      for (const auto& io_data : {msg, msg, msg, big_msg, msg, msg, big_msg,
                                  msg, big_msg, msg, msg}) {
        send_bytes += tls_client.WriteAll(io_data.data, io_data.len, deadline);
      }
      benchmark::DoNotOptimize(send_bytes);
    }

    reading.store(false);
    server_task.Get();
  });
}

BENCHMARK(tls_write_all_default)
    ->RangeMultiplier(2)
    ->Range(1 << 6, 1 << 12)
    ->Unit(benchmark::kNanosecond);

USERVER_NAMESPACE_END
