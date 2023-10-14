#pragma once

#include <functional>
#include <memory>
#include <string>

#include <server/http/request_handler_base.hpp>
#include <server/net/connection_config.hpp>
#include <server/net/stats.hpp>
#include <server/request/request_parser.hpp>

#include <userver/concurrent/queue.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/server/request/request_base.hpp>
#include <userver/server/request/request_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

class Connection final {
 public:
  enum class Type { kRequest, kMonitor };

  Connection(const ConnectionConfig& config,
             const request::HttpRequestConfig& handler_defaults_config,
             std::unique_ptr<engine::io::RwBase> peer_socket,
             const engine::io::Sockaddr& remote_address,
             const http::RequestHandlerBase& request_handler,
             std::shared_ptr<Stats> stats,
             request::ResponseDataAccounter& data_accounter);

  void Process();

  int Fd() const;

 private:
  using QueueItem = std::pair<std::shared_ptr<request::RequestBase>,
                              engine::TaskWithResult<void>>;
  using Queue = concurrent::SpscQueue<QueueItem>;

  void Shutdown() noexcept;

  bool IsRequestTasksEmpty() const noexcept;

  void ListenForRequests(Queue::Producer producer,
                         engine::TaskCancellationToken token) noexcept;
  bool NewRequest(std::shared_ptr<request::RequestBase>&& request_ptr,
                  Queue::Producer&);

  void ProcessResponses(Queue::Consumer&) noexcept;
  void HandleQueueItem(QueueItem& item) noexcept;
  void SendResponse(request::RequestBase& request);

  std::string Getpeername() const;

  const ConnectionConfig& config_;
  const request::HttpRequestConfig& handler_defaults_config_;
  std::unique_ptr<engine::io::RwBase> peer_socket_;
  const http::RequestHandlerBase& request_handler_;
  const std::shared_ptr<Stats> stats_;
  request::ResponseDataAccounter& data_accounter_;

  engine::io::Sockaddr remote_address_;
  std::string peer_name_;

  std::shared_ptr<Queue> request_tasks_;

  bool is_accepting_requests_{true};
  bool is_response_chain_valid_{true};
};

}  // namespace server::net

USERVER_NAMESPACE_END
