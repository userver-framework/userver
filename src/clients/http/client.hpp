#pragma once

#include <mutex>
#include <queue>

#include <curl-ev/multi.hpp>

#include "request.hpp"

namespace clients {
namespace http {

class Client : public std::enable_shared_from_this<Client> {
 public:
  /* Use this method to create Client */
  static std::shared_ptr<Client> Create(size_t io_threads);
  explicit Client(size_t io_threads);

  Client(const Client&) = delete;
  Client(Client&&) = delete;
  ~Client();

  void Stop();

  std::shared_ptr<Request> CreateRequest();

  void SetMaxPipelineLength(size_t max_pipeline_length);
  void SetMaxHostConnections(size_t max_host_connections);
  void SetConnectionPoolSize(size_t connection_pool_size);

  void PushIdleEasy(std::shared_ptr<curl::easy> easy);

 private:
  std::vector<std::shared_ptr<curl::multi>> multis_;
  engine::ev::ThreadPool thread_pool_;

  std::mutex idle_easy_queue_mutex_;
  std::queue<std::shared_ptr<curl::easy>> idle_easy_queue_;
};

}  // namespace http
}  // namespace clients
