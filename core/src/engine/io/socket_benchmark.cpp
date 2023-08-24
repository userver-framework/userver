#include <benchmark/benchmark.h>

#include <array>
#include <chrono>
#include <string>

#include <userver/engine/async.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/internal/net/net_listener.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

using Deadline = engine::Deadline;

namespace {

constexpr auto kDeadlineMaxTime = std::chrono::seconds{60};

}  // namespace

void socket_send_all(benchmark::State& state) {
  engine::RunStandalone([&]() {
    const auto test_deadline = Deadline::FromDuration(kDeadlineMaxTime);
    internal::net::TcpListener listener;
    auto [server, client] = listener.MakeSocketPair(test_deadline);
    std::atomic<bool> reading{true};
    auto task_reader = engine::AsyncNoSpan(
        [&reading, test_deadline](auto&& server) {
          std::array<char, 128> buf = {};
          while (server.RecvSome(buf.data(), buf.size(), test_deadline) > 0 &&
                 reading) {
          }
        },
        std::move(server));
    for (auto _ : state) {
      auto send_bytes = client.SendAll("qqq", 3, test_deadline);
      send_bytes += client.SendAll("aaa", 3, test_deadline);
      send_bytes += client.SendAll("qwerty", 6, test_deadline);
      benchmark::DoNotOptimize(send_bytes);
    }
    reading.store(false);
    task_reader.Get();
  });
}
BENCHMARK(socket_send_all);

void socket_send_all_v(benchmark::State& state) {
  engine::RunStandalone([&]() {
    const auto test_deadline = Deadline::FromDuration(kDeadlineMaxTime);
    internal::net::TcpListener listener;
    auto [server, client] = listener.MakeSocketPair(test_deadline);
    std::atomic<bool> reading{true};
    auto task_reader = engine::AsyncNoSpan(
        [&reading, test_deadline](auto&& server) {
          std::array<char, 128> buf = {};
          while (server.RecvSome(buf.data(), buf.size(), test_deadline) > 0 &&
                 reading) {
          }
        },
        std::move(server));
    for (auto _ : state) {
      const auto send_bytes = client.SendAll(
          {{"qqq", 3}, {"aaa", 3}, {"qwerty", 6}}, test_deadline);
      benchmark::DoNotOptimize(send_bytes);
    }
    reading.store(false);
    task_reader.Get();
  });
}
BENCHMARK(socket_send_all_v);

[[maybe_unused]] void socket_send_all_v_range(benchmark::State& state) {
  engine::RunStandalone(2, [&]() {
    const auto test_deadline = Deadline::FromDuration(kDeadlineMaxTime);
    internal::net::TcpListener listener;
    const auto send_buff = std::string(state.range(0), 'a');
    const auto size_buff = fmt::format("\r\n{:x}\r\n", send_buff.size());
    auto [server, client] = listener.MakeSocketPair(test_deadline);
    std::atomic<bool> reading{true};
    auto task_reader = engine::AsyncNoSpan(
        [&reading, test_deadline](auto&& server) {
          std::array<char, 128> buf = {};
          while (server.RecvSome(buf.data(), buf.size(), test_deadline) > 0 &&
                 reading) {
          }
        },
        std::move(server));
    for (auto _ : state) {
      const auto send_bytes =
          client.SendAll({{size_buff.data(), size_buff.size()},
                          {send_buff.data(), send_buff.size()}},
                         test_deadline);
      benchmark::DoNotOptimize(send_bytes);
    }
    reading.store(false);
    task_reader.Get();
  });
}

// TODO(TAXICOMMON-5510) flaky, sometimes throws engine::io::IoTimeout
// BENCHMARK(socket_send_all_v_range)->RangeMultiplier(10)->Range(10, 10000);

[[maybe_unused]] void socket_send_all_range(benchmark::State& state) {
  engine::RunStandalone(2, [&]() {
    const auto test_deadline = Deadline::FromDuration(kDeadlineMaxTime);
    internal::net::TcpListener listener;
    const auto send_buff = std::string(state.range(0), 'a');
    const auto size_buff = fmt::format("\r\n{:x}\r\n", send_buff.size());
    auto [server, client] = listener.MakeSocketPair(test_deadline);
    std::atomic<bool> reading{true};
    auto task_reader = engine::AsyncNoSpan(
        [&reading, test_deadline](auto&& server) {
          std::array<char, 128> buf = {};
          while (server.RecvSome(buf.data(), buf.size(), test_deadline) > 0 &&
                 reading) {
          }
        },
        std::move(server));
    for (auto _ : state) {
      auto send_bytes =
          client.SendAll(size_buff.data(), size_buff.size(), test_deadline);
      send_bytes +=
          client.SendAll(send_buff.data(), send_buff.size(), test_deadline);
      benchmark::DoNotOptimize(send_bytes);
    }
    reading.store(false);
    task_reader.Get();
  });
}

// TODO(TAXICOMMON-5510) flaky, sometimes throws engine::io::IoTimeout
// BENCHMARK(socket_send_all_range)->RangeMultiplier(10)->Range(10, 10000);

USERVER_NAMESPACE_END
