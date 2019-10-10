#include <clients/http/client.hpp>

#include <clients/http/destination_statistics.hpp>
#include <clients/http/testsuite.hpp>
#include <curl-ev/multi.hpp>
#include <engine/ev/thread_pool.hpp>
#include <logging/log.hpp>

#include <moodycamel/concurrentqueue.h>

#include <cstdlib>

#include <utils/openssl_lock.hpp>

namespace clients {
namespace http {
namespace {

const std::string kIoThreadName = "curl";

}  // namespace

std::shared_ptr<Client> Client::Create(const std::string& thread_name_prefix,
                                       size_t io_threads) {
  struct EmplaceEnabler : public Client {
    EmplaceEnabler(const std::string& thread_name_prefix, size_t io_threads)
        : Client(thread_name_prefix, io_threads) {}
  };
  return std::make_shared<EmplaceEnabler>(thread_name_prefix, io_threads);
}

Client::Client(const std::string& thread_name_prefix, size_t io_threads)
    : destination_statistics_(std::make_shared<DestinationStatistics>()),
      statistics_(io_threads),
      idle_queue_() {
  engine::ev::ThreadPoolConfig ev_config;
  ev_config.threads = io_threads;
  ev_config.thread_name =
      kIoThreadName +
      (thread_name_prefix.empty() ? "" : ("-" + thread_name_prefix));
  thread_pool_ = std::make_unique<engine::ev::ThreadPool>(std::move(ev_config));

  multis_.reserve(io_threads);
  for (auto thread_control_ptr : thread_pool_->NextThreads(io_threads)) {
    multis_.push_back(std::make_unique<curl::multi>(*thread_control_ptr));
  }
}

Client::~Client() {
  // We have to destroy *this only when all the requests are finished, because
  // otherwise `multis_` and `thread_pool_` are destroyed and pending requests
  // cause UB (e.g. segfault).
  //
  // We can not refcount *this in `EasyWrapper` because that leads to
  // destruction of `thread_pool_` in the thread of the thread pool (that's an
  // UB).
  //
  // Best solution so far: track the pending tasks and wait for them to drop to
  // zero.
  while (pending_tasks_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  while (TryDequeueIdle())
    ;

  multis_.clear();
  thread_pool_.reset();
}

std::shared_ptr<Request> Client::CreateRequest() {
  std::shared_ptr<Request> request;

  auto easy = TryDequeueIdle();
  if (easy) {
    auto idx = FindMultiIndex(easy->GetMulti());
    auto wrapper = std::make_shared<EasyWrapper>(std::move(easy), *this);
    request = std::make_shared<Request>(std::move(wrapper),
                                        statistics_[idx].CreateRequestStats(),
                                        destination_statistics_);
  } else {
    thread_local unsigned int rand_state = 0;
    int i = rand_r(&rand_state) % multis_.size();
    auto& multi = multis_[i];
    auto wrapper = std::make_shared<EasyWrapper>(
        std::make_shared<curl::easy>(*multi), *this);
    request = std::make_shared<Request>(std::move(wrapper),
                                        statistics_[i].CreateRequestStats(),
                                        destination_statistics_);
  }

  if (testsuite_config_) {
    request->SetTestsuiteConfig(testsuite_config_);
  }

  return request;
}

void Client::SetMaxPipelineLength(size_t max_pipeline_length) {
  curl::multi::pipelining_mode_t mode =
      max_pipeline_length > 1
          ? static_cast<curl::multi::pipelining_mode_t>(
                curl::multi::pipe_multiplex | curl::multi::pipe_http1)
          : curl::multi::pipe_nothing;
  for (auto& multi : multis_) {
    multi->set_pipelining(mode);
    multi->set_max_pipeline_length(max_pipeline_length);
  }
}

void Client::SetMaxHostConnections(size_t max_host_connections) {
  for (auto& multi : multis_) {
    multi->set_max_host_connections(max_host_connections);
  }
}

void Client::SetConnectionPoolSize(size_t connection_pool_size) {
  const size_t pool_size = connection_pool_size / multis_.size();
  if (pool_size * multis_.size() != connection_pool_size) {
    LOG_WARNING()
        << "SetConnectionPoolSize() rounded pool size for each multi ("
        << connection_pool_size << "/" << multis_.size() << " rounded to "
        << pool_size << ")";
  }
  for (auto& multi : multis_) {
    multi->set_max_connections(pool_size);
  }
}

InstanceStatistics Client::GetMultiStatistics(size_t n) const {
  UASSERT(n < statistics_.size());
  InstanceStatistics s(statistics_[n]);

  /* Update statistics from multi in place */
  const auto& multi_stats = multis_[n]->Statistics();

  /* There is a race between close/open updates, so at least make open>=close
   * to observe non-negative current socket count. */
  s.multi.socket_close = multi_stats.close_socket_total();
  s.multi.socket_open = multi_stats.open_socket_total();
  s.multi.current_load = multi_stats.get_busy_storage().GetCurrentLoad();
  return s;
}

size_t Client::FindMultiIndex(const curl::multi& multi) const {
  for (size_t i = 0; i < multis_.size(); i++) {
    if (multis_[i].get() == &multi) return i;
  }
  UASSERT_MSG(false, "Unknown multi");
  throw std::logic_error("Unknown multi");
}

PoolStatistics Client::GetPoolStatistics() const {
  PoolStatistics stats;
  stats.multi.reserve(multis_.size());
  for (size_t i = 0; i < multis_.size(); i++) {
    stats.multi.push_back(GetMultiStatistics(i));
  };
  return stats;
}

void Client::SetDestinationMetricsAutoMaxSize(size_t max_size) {
  destination_statistics_->SetAutoMaxSize(max_size);
}

const DestinationStatistics& Client::GetDestinationStatistics() const {
  return *destination_statistics_;
}

void Client::PushIdleEasy(std::shared_ptr<curl::easy> easy) noexcept {
  try {
    easy->reset();
    idle_queue_->enqueue(std::move(easy));
  } catch (const std::exception& e) {
    LOG_ERROR() << e;
  }

  DecPending();
}

std::shared_ptr<curl::easy> Client::TryDequeueIdle() noexcept {
  std::shared_ptr<curl::easy> result;
  if (!idle_queue_->try_dequeue(result)) {
    return {};
  }
  return result;
}

void Client::SetTestsuiteConfig(const TestsuiteConfig& config) {
  LOG_INFO() << "http client: configured for testsuite";
  testsuite_config_ = std::make_shared<const TestsuiteConfig>(config);
}

}  // namespace http
}  // namespace clients
