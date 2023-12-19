#include <userver/utest/using_namespace_userver.hpp>

/// [TCP sample - component]
#include <userver/clients/dns/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/components/tcp_acceptor_base.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/utils/statistics/metric_tag.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>

namespace samples::tcp::echo {

struct Stats;

class Echo final : public components::TcpAcceptorBase {
 public:
  static constexpr std::string_view kName = "tcp-echo";

  // Component is valid after construction and is able to accept requests
  Echo(const components::ComponentConfig& config,
       const components::ComponentContext& context);

  void ProcessSocket(engine::io::Socket&& sock) override;

 private:
  Stats& stats_;
};

}  // namespace samples::tcp::echo

/// [TCP sample - component]

namespace samples::tcp::echo {

/// [TCP sample - Stats definition]
struct Stats {
  std::atomic<std::uint64_t> opened_sockets{0};
  std::atomic<std::uint64_t> closed_sockets{0};
  std::atomic<std::uint64_t> bytes_read{0};
};
/// [TCP sample - Stats definition]

/// [TCP sample - Stats tag]
const utils::statistics::MetricTag<Stats> kTcpEchoTag{"tcp-echo"};

void DumpMetric(utils::statistics::Writer& writer, const Stats& stats) {
  writer["sockets"]["opened"] = stats.opened_sockets;
  writer["sockets"]["closed"] = stats.closed_sockets;
  writer["bytes"]["read"] = stats.bytes_read;
}

void ResetMetric(Stats& stats) {
  stats.opened_sockets = 0;
  stats.closed_sockets = 0;
  stats.bytes_read = 0;
}
/// [TCP sample - Stats tag]

/// [TCP sample - constructor]
Echo::Echo(const components::ComponentConfig& config,
           const components::ComponentContext& context)
    : TcpAcceptorBase(config, context),
      stats_(context.FindComponent<components::StatisticsStorage>()
                 .GetMetricsStorage()
                 ->GetMetric(kTcpEchoTag)) {}
/// [TCP sample - constructor]

/// [TCP sample - SendRecv]
namespace {

using Queue = concurrent::SpscQueue<std::string>;

void DoSend(engine::io::Socket& sock, Queue::Consumer consumer) {
  std::string data;
  while (consumer.Pop(data)) {
    const auto sent_bytes = sock.SendAll(data.data(), data.size(), {});
    if (sent_bytes != data.size()) {
      LOG_INFO() << "Failed to send all the data";
      return;
    }
  }
}

void DoRecv(engine::io::Socket& sock, Queue::Producer producer, Stats& stats) {
  std::array<char, 1024> buf;  // NOLINT(cppcoreguidelines-pro-type-member-init)
  while (!engine::current_task::ShouldCancel()) {
    const auto read_bytes = sock.ReadSome(buf.data(), buf.size(), {});
    if (!read_bytes) {
      LOG_INFO() << "Failed to read data";
      return;
    }

    stats.bytes_read += read_bytes;
    if (!producer.Push({buf.data(), read_bytes})) {
      return;
    }
  }
}

}  // anonymous namespace
/// [TCP sample - SendRecv]

/// [TCP sample - ProcessSocket]
void Echo::ProcessSocket(engine::io::Socket&& sock) {
  const auto sock_num = ++stats_.opened_sockets;

  tracing::Span span{fmt::format("sock_{}", sock_num)};
  span.AddTag("fd", std::to_string(sock.Fd()));

  utils::FastScopeGuard guard{[this]() noexcept {
    LOG_INFO() << "Closing socket";
    ++stats_.closed_sockets;
  }};

  auto queue = Queue::Create();

  auto send_task =
      utils::Async("send", DoSend, std::ref(sock), queue->GetConsumer());

  DoRecv(sock, queue->GetProducer(), stats_);
}
/// [TCP sample - ProcessSocket]

}  // namespace samples::tcp::echo

/// [TCP sample - main]
int main(int argc, const char* const argv[]) {
  const auto component_list = components::MinimalServerComponentList()
                                  .Append<server::handlers::ServerMonitor>()
                                  .Append<samples::tcp::echo::Echo>()
                                  // Testuite components:
                                  .Append<server::handlers::TestsControl>()
                                  .Append<components::TestsuiteSupport>()
                                  .Append<clients::dns::Component>()
                                  .Append<components::HttpClient>();

  return utils::DaemonMain(argc, argv, component_list);
}
/// [TCP sample - main]
