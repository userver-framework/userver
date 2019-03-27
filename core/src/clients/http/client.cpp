#include <clients/http/client.hpp>
#include <curl-ev/multi.hpp>
#include <engine/ev/thread_pool.hpp>
#include <logging/log.hpp>

#include <cstdlib>

#include <utils/openssl_lock.hpp>

namespace clients {
namespace http {
namespace {

const std::string kIoThreadName = "curl";

}  // namespace

std::shared_ptr<Client> Client::Create(size_t io_threads) {
  struct EmplaceEnabler : public Client {
    EmplaceEnabler(size_t io_threads) : Client(io_threads) {}
  };
  return std::make_shared<EmplaceEnabler>(io_threads);
}

Client::Client(size_t io_threads) {
  engine::ev::ThreadPoolConfig ev_config;
  ev_config.threads = io_threads;
  ev_config.thread_name = kIoThreadName;
  thread_pool_ = std::make_unique<engine::ev::ThreadPool>(std::move(ev_config));

  for (auto thread_control_ptr : thread_pool_->NextThreads(io_threads)) {
    multis_.push_back(std::make_unique<curl::multi>(*thread_control_ptr));
  }
}

Client::~Client() {
  {
    std::lock_guard<std::mutex> lock(idle_easy_queue_mutex_);
    while (!idle_easy_queue_.empty()) idle_easy_queue_.pop();
  }

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

  multis_.clear();
  thread_pool_.reset();
}

std::shared_ptr<Request> Client::CreateRequest() {
  {
    std::unique_lock<std::mutex> lock(idle_easy_queue_mutex_);
    if (!idle_easy_queue_.empty()) {
      auto easy = std::move(idle_easy_queue_.front());
      idle_easy_queue_.pop();
      lock.unlock();

      auto wrapper = std::make_shared<EasyWrapper>(std::move(easy), *this);
      return std::make_shared<Request>(std::move(wrapper));
    }
  }

  thread_local unsigned int rand_state = 0;
  int i = rand_r(&rand_state) % multis_.size();
  auto& multi = multis_[i];
  auto wrapper = std::make_shared<EasyWrapper>(
      std::make_unique<curl::easy>(*multi), *this);
  return std::make_shared<Request>(std::move(wrapper));
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

void Client::PushIdleEasy(std::unique_ptr<curl::easy> easy) noexcept {
  try {
    easy->reset();
    std::lock_guard<std::mutex> lock(idle_easy_queue_mutex_);
    idle_easy_queue_.push(std::move(easy));
  } catch (const std::exception& e) {
    LOG_ERROR() << e.what();
  }

  DecPending();
}

}  // namespace http
}  // namespace clients
