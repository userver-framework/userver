#include <clients/dns/net_resolver.hpp>

#include <sys/select.h>

#include <array>
#include <chrono>
#include <exception>
#include <fstream>
#include <limits>
#include <memory>
#include <unordered_map>

#include <ares.h>
#include <fmt/format.h>
#include <moodycamel/concurrentqueue.h>

#include <clients/dns/helpers.hpp>
#include <clients/dns/wrappers.hpp>
#include <engine/io/poller.hpp>
#include <userver/clients/dns/common.hpp>
#include <userver/clients/dns/exception.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/mock_now.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {
namespace {

// Sockets poll timeout to avoid ares internal pipeline stalling
constexpr auto kMaxAresProcessDelay = std::chrono::milliseconds{100};

std::exception_ptr MakeNotResolvedException(std::string_view name,
                                            std::string_view reason) {
  return std::make_exception_ptr(NotResolvedException{
      fmt::format("Could not resolve {}: {}", name, reason)});
}

}  // namespace

class NetResolver::Impl {
 public:
  struct Request {
    std::string name;
    engine::Promise<Response> promise;
  };

  engine::io::Poller poller;
  impl::ChannelPtr channel;
  moodycamel::ConcurrentQueue<std::unique_ptr<Request>> requests_queue;
  engine::Task worker_task;

  static void SockStateCallback(void* data, ares_socket_t socket_fd,
                                int readable, int writable) noexcept {
    auto* self = static_cast<Impl*>(data);
    UASSERT(self);
    if (!readable && !writable) {
      // Socket is closing, ensure no watchers are running
      self->poller.Remove(socket_fd);
    }
  }

  static void AddrinfoCallback(void* arg, int status, int /* timeouts */,
                               struct ares_addrinfo* result) {
    std::unique_ptr<Request> request{static_cast<Request*>(arg)};
    UASSERT(request);

    Response response;
    response.received_at = utils::datetime::MockNow();
    if (status != ARES_SUCCESS) {
      request->promise.set_exception(
          MakeNotResolvedException(request->name, ares_strerror(status)));
      return;
    }
    UASSERT(result);
    impl::AddrinfoPtr ai{result};
    // RFC2181 5.2: Consequently the use of differing TTLs in an RRSet is hereby
    // deprecated, the TTLs of all RRs in an RRSet must be the same. [...]
    // Should an authoritative source send such a malformed RRSet, the client
    // should treat the RRs for all purposes as if all TTLs in the RRSet had
    // been set to the value of the lowest TTL in the RRSet.
    //
    // NOTE: We're working with different RRSets here, but it's easier for
    // caching purposes to process them together.
    response.ttl = std::chrono::seconds::max();
    for (auto* node = ai->nodes; node; node = node->ai_next) {
      response.addrs.emplace_back(node->ai_addr);
      if (response.ttl.count() > node->ai_ttl) {
        response.ttl = std::chrono::seconds{node->ai_ttl};
      }
      LOG_DEBUG() << request->name << " resolved to " << response.addrs.back()
                  << ", ttl=" << node->ai_ttl;
    }
    if (response.addrs.empty()) {
      request->promise.set_exception(
          MakeNotResolvedException(request->name, "Empty address list"));
      return;
    }
    impl::SortAddrs(response.addrs);
    request->promise.set_value(std::move(response));
  }

  void AddSocketEventsToPoller() {
    std::array<ares_socket_t, ARES_GETSOCK_MAXNUM> ares_sockets{};
    const auto mask =
        ::ares_getsock(channel.get(), ares_sockets.data(), ares_sockets.size());
    for (size_t i = 0; i < ares_sockets.size(); ++i) {
      utils::Flags<engine::io::Poller::Event::Type> events;
      if (ARES_GETSOCK_READABLE(mask, i)) {
        events |= engine::io::Poller::Event::kRead;
      }
      if (ARES_GETSOCK_WRITABLE(mask, i)) {
        events |= engine::io::Poller::Event::kWrite;
      }
      if (events) {
        poller.Add(ares_sockets[i], events);
      }
    }
  }

  void PollEvents() {
    engine::io::Poller::Event poller_event;
    auto poll_status = poller.NextEvent(
        poller_event, engine::Deadline::FromDuration(kMaxAresProcessDelay));
    while (poll_status != engine::io::Poller::Status::kNoEvents) {
      if (poll_status == engine::io::Poller::Status::kInterrupt) {
        LOG_TRACE() << "Got an interrupt";
      } else if (poller_event.fd == engine::io::kInvalidFd) {
        LOG_LIMITED_WARNING() << "Got an event for the invalid fd";
      } else {
        ::ares_socket_t read_fd = ARES_SOCKET_BAD;
        ::ares_socket_t write_fd = ARES_SOCKET_BAD;

        LOG_TRACE() << "Got an event for fd " << poller_event.fd;
        if (poller_event.type & engine::io::Poller::Event::kRead) {
          read_fd = poller_event.fd;
        }
        if (poller_event.type & engine::io::Poller::Event::kWrite) {
          write_fd = poller_event.fd;
        }
        if (poller_event.type & engine::io::Poller::Event::kError) {
          write_fd = poller_event.fd;
          read_fd = poller_event.fd;
        }

        ::ares_process_fd(channel.get(), read_fd, write_fd);
      }
      poll_status = poller.NextEventNoblock(poller_event);
    }
  }

  void Worker() {
    static constexpr struct ares_addrinfo_hints kHints {
      /*ai_flags=*/ARES_AI_NUMERICSERV | ARES_AI_NOSORT,
          /*ai_family=*/AF_UNSPEC, /*ai_socktype=*/0, /*ai_protocol=*/0
    };

    moodycamel::ConsumerToken requests_queue_token{requests_queue};
    std::vector<std::unique_ptr<Request>> current_requests;
    while (!engine::current_task::ShouldCancel()) {
      current_requests.clear();
      requests_queue.try_dequeue_bulk(requests_queue_token,
                                      std::back_inserter(current_requests), -1);
      for (auto& req : current_requests) {
        const auto* name_c_str = req->name.c_str();
        ::ares_getaddrinfo(channel.get(), name_c_str, nullptr, &kHints,
                           &AddrinfoCallback, req.release());
      }

      AddSocketEventsToPoller();
      PollEvents();

      // process timeouts even if no events
      ::ares_process_fd(channel.get(), ARES_SOCKET_BAD, ARES_SOCKET_BAD);
    }
  }
};

NetResolver::NetResolver(engine::TaskProcessor& fs_task_processor,
                         std::chrono::milliseconds query_timeout,
                         int query_attempts,
                         const std::vector<std::string>& custom_servers) {
  static const impl::GlobalInitializer kInitCAres;

  if (query_timeout.count() < 0 ||
      query_timeout.count() > std::numeric_limits<int>::max()) {
    throw ResolverException(
        "Invalid network resolver config: timeout must be positive and less "
        "than 24 days");
  }

  if (query_attempts < 1) {
    throw ResolverException(
        "Invalid network resolver config: number of attempts must be positive");
  }

  constexpr int kOptmask = ARES_OPT_FLAGS | ARES_OPT_TIMEOUTMS |
                           ARES_OPT_TRIES | ARES_OPT_DOMAINS |
                           ARES_OPT_LOOKUPS | ARES_OPT_SOCK_STATE_CB;
  struct ares_options options {};
  options.flags = ARES_FLAG_STAYOPEN |  // do not close idle sockets
                  ARES_FLAG_NOALIASES;  // ignore HOSTALIASES from env
  options.timeout = query_timeout.count();
  options.tries = query_attempts;
  options.domains = nullptr;
  options.ndomains = 0;
  options.sock_state_cb = &Impl::SockStateCallback;
  options.sock_state_cb_data = &*impl_;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  options.lookups = const_cast<char*>("b");  // network lookups only

  impl_->channel =
      utils::Async(fs_task_processor, "net-resolver-init", [&options] {
        ares_channel channel = nullptr;
        const int init_ret = ::ares_init_options(&channel, &options, kOptmask);
        if (init_ret != ARES_SUCCESS) {
          throw ResolverException(fmt::format(
              "Failed to create ares channel: {}", ::ares_strerror(init_ret)));
        }
        return impl::ChannelPtr{channel};
      }).Get();

  if (!custom_servers.empty()) {
    auto servers_csv = fmt::to_string(fmt::join(custom_servers, ","));
    const int set_servers_ret =
        ::ares_set_servers_ports_csv(impl_->channel.get(), servers_csv.c_str());
    if (set_servers_ret != ARES_SUCCESS) {
      throw ResolverException(
          fmt::format("Cannot install custom server list: {}",
                      ::ares_strerror(set_servers_ret)));
    }
  }

  // NOLINTNEXTLINE(cppcoreguidelines-slicing)
  impl_->worker_task = engine::AsyncNoSpan([this] { impl_->Worker(); });
}

NetResolver::~NetResolver() = default;

engine::Future<NetResolver::Response> NetResolver::Resolve(std::string name) {
  auto request = std::make_unique<Impl::Request>();
  request->name = std::move(name);
  auto future = request->promise.get_future();
  impl_->requests_queue.enqueue(std::move(request));
  impl_->poller.Interrupt();
  return future;
}

}  // namespace clients::dns

USERVER_NAMESPACE_END
