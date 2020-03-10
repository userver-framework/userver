#pragma once

#include <functional>
#include <memory>
#include <string>

#include <server/request/request_base.hpp>

#include <engine/io/socket.hpp>
#include <engine/mpsc_queue.hpp>
#include <engine/single_consumer_event.hpp>
#include <engine/task/task.hpp>
#include <engine/task/task_processor.hpp>
#include <server/http/request_handler_base.hpp>
#include <server/net/stats.hpp>
#include <server/request/request_parser.hpp>
#include <utils/work_control.hpp>

#include "connection_config.hpp"

namespace server::net {

class Connection final : public std::enable_shared_from_this<Connection> {
  struct EmplaceEnabler {};

 public:
  using CloseCb = std::function<void()>;

  enum class Type { kRequest, kMonitor };

  static std::shared_ptr<Connection> Create(
      engine::TaskProcessor& task_processor, const ConnectionConfig& config,
      engine::io::Socket peer_socket,
      const http::RequestHandlerBase& request_handler,
      std::shared_ptr<Stats> stats);

  // Use Create instead of this constructor
  Connection(engine::TaskProcessor& task_processor,
             const ConnectionConfig& config, engine::io::Socket peer_socket,
             const http::RequestHandlerBase& request_handler,
             std::shared_ptr<Stats> stats, EmplaceEnabler);

  void SetCloseCb(CloseCb close_cb);

  void Start();

  void Stop();  // Can be called after Start() has finished

  int Fd() const;

 private:
  using QueueItem = std::pair<std::shared_ptr<request::RequestBase>,
                              engine::TaskWithResult<void>>;
  using Queue = engine::MpscQueue<std::unique_ptr<QueueItem>>;

  void Shutdown() noexcept;

  bool IsRequestTasksEmpty() const noexcept;

  void ListenForRequests(Queue::Producer) noexcept;
  bool NewRequest(std::shared_ptr<request::RequestBase>&& request_ptr,
                  Queue::Producer&);

  void ProcessResponses(Queue::Consumer&) noexcept;
  void HandleQueueItem(QueueItem& item);
  void SendResponse(request::RequestBase& request);

 private:
  engine::TaskProcessor& task_processor_;
  const ConnectionConfig& config_;
  engine::io::Socket peer_socket_;
  const http::RequestHandlerBase& request_handler_;
  const std::shared_ptr<Stats> stats_;
  const std::string remote_address_;

  std::shared_ptr<Queue> request_tasks_;
  engine::SingleConsumerEvent response_sender_launched_event_;
  engine::SingleConsumerEvent response_sender_assigned_event_;
  engine::Task response_sender_task_;

  bool is_accepting_requests_;
  bool is_response_chain_valid_;
  CloseCb close_cb_;
};

}  // namespace server::net
