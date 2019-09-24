#include "connection.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <system_error>
#include <vector>

#include <boost/lexical_cast.hpp>

#include <engine/async.hpp>
#include <engine/io/exception.hpp>
#include <engine/task/cancel.hpp>
#include <logging/log.hpp>
#include <server/http/http_request_handler.hpp>
#include <server/http/http_request_parser.hpp>
#include <server/request/request_config.hpp>
#include <utils/assert.hpp>

namespace server {
namespace net {

Connection::Connection(engine::TaskProcessor& task_processor,
                       const ConnectionConfig& config,
                       engine::io::Socket peer_socket,
                       const http::HttpRequestHandler& request_handler,
                       std::shared_ptr<Stats> stats)
    : task_processor_(task_processor),
      config_(config),
      peer_socket_(std::move(peer_socket)),
      request_handler_(request_handler),
      stats_(std::move(stats)),
      remote_address_(peer_socket_.Getpeername().RemoteAddress()),
      request_tasks_(Queue::Create()),
      is_accepting_requests_(true),
      stop_sender_after_queue_is_empty_(false),
      is_closing_(false) {
  LOG_DEBUG() << "Incoming connection from " << peer_socket_.Getpeername()
              << ", fd " << Fd();

  ++stats_->active_connections;
  ++stats_->connections_created;
}

Connection::~Connection() {
  const int fd = Fd();  // will be invalidated on close

  // Socket listener can be simply cancelled
  LOG_TRACE() << "Stopping socket listener for fd " << fd;
  UASSERT(socket_listener_.IsValid());
  socket_listener_ = {};
  LOG_TRACE() << "Stopped socket listener for fd " << fd;

  // SendResponses() has to wait for all responses and send them out
  LOG_TRACE() << "Waiting for request tasks queue to become empty for fd "
              << fd;
  response_sender_task_.Wait();

  peer_socket_.Close();

  LOG_TRACE() << "RequestBase tasks queue became empty for fd " << fd;
  UASSERT(IsRequestTasksEmpty());

  --stats_->active_connections;
  ++stats_->connections_closed;

  if (close_cb_) close_cb_();
}

void Connection::SetCloseCb(CloseCb close_cb) {
  close_cb_ = std::move(close_cb);
}

void Connection::Start() {
  shared_this_ = shared_from_this();

  LOG_TRACE() << "Starting socket listener for fd " << Fd();

  response_sender_task_ = engine::impl::CriticalAsync(
      task_processor_,
      [this](Queue::Consumer consumer) { SendResponses(std::move(consumer)); },
      request_tasks_->GetConsumer());
  socket_listener_ =
      engine::impl::CriticalAsync(task_processor_,
                                  [this](Queue::Producer producer) {
                                    ListenForRequests(std::move(producer));
                                  },
                                  request_tasks_->GetProducer());
  LOG_TRACE() << "Started socket listener for fd " << Fd();
}

void Connection::Stop() { socket_listener_.RequestCancel(); }

int Connection::Fd() const { return peer_socket_.Fd(); }

bool Connection::IsRequestTasksEmpty() const {
  return request_tasks_->Size() == 0;
}

void Connection::ListenForRequests(Queue::Producer producer) {
  request_tasks_->SetMaxLength(config_.requests_queue_size_threshold);
  try {
    http::HttpRequestParser request_parser(
        request_handler_.GetHandlerInfoIndex(), *config_.request,
        [this, &producer](std::shared_ptr<request::RequestBase>&& request_ptr) {
          if (!NewRequest(std::move(request_ptr), producer))
            is_accepting_requests_ = false;
        },
        stats_->parser_stats);

    std::vector<char> buf(config_.in_buffer_size);
    while (is_accepting_requests_) {
      size_t bytes_read = peer_socket_.RecvSome(buf.data(), buf.size(), {});
      if (!bytes_read) {
        LOG_TRACE() << "Peer " << peer_socket_.Getpeername() << " on fd "
                    << Fd() << "closed connection";
        break;
      }
      LOG_TRACE() << "Received " << bytes_read << " byte(s) from "
                  << peer_socket_.Getpeername() << " on fd " << Fd();

      if (!request_parser.Parse(buf.data(), bytes_read)) {
        LOG_DEBUG() << "Malformed request from " << peer_socket_.Getpeername()
                    << " on fd " << Fd();
        break;
      }
    }
  } catch (const engine::io::IoCancelled&) {
    // go to finalization, do not pass Go, do not collect $200
  } catch (const engine::io::IoSystemError& ex) {
    // working with raw values because std::errc compares error_category
    // default_error_category() fixed only in GCC 9.1 (PR libstdc++/60555)
    auto log_level =
        ex.Code().value() == static_cast<int>(std::errc::connection_reset)
            ? logging::Level::kWarning
            : logging::Level::kError;
    LOG(log_level) << "I/O error while receiving from peer "
                   << peer_socket_.Getpeername() << " on fd " << Fd() << ": "
                   << ex;
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Error while receiving from peer "
                << peer_socket_.Getpeername() << " on fd " << Fd() << ": "
                << ex;
  }

  LOG_TRACE() << "Stopping ListenForRequests()";
  CloseAsync();
}

bool Connection::NewRequest(std::shared_ptr<request::RequestBase>&& request_ptr,
                            Queue::Producer& producer) {
  if (!is_accepting_requests_) {
    /* In case of recv() of >1 requests it is possible to get here
     * after is_accepting_requests_ is set to true. Just ignore tail
     * garbage.
     */
    return true;
  }

  if (request_ptr->IsFinal()) {
    is_accepting_requests_ = false;
  }

  ++stats_->active_request_count;
  return producer.Push(std::make_unique<QueueItem>(
      request_ptr, request_handler_.StartRequestTask(request_ptr)));
}

void Connection::HandleQueueItem(QueueItem& item) {
  try {
    item.second.Get();
  } catch (const std::exception& e) {
    LOG_WARNING() << "Request failed with unhandled exception: " << e;

    item.first->MarkAsInternalServerError();
  }
}

void Connection::SendResponses(Queue::Consumer consumer) {
  LOG_TRACE() << "Sending responses for fd " << Fd();

  while (!engine::current_task::ShouldCancel()) {
    std::unique_ptr<QueueItem> item;
    if (!consumer.Pop(item)) break;

    HandleQueueItem(*item);
    auto& request = *item->first;
    auto& response = request.GetResponse();

    // the response is ready, so let's not drop it
    engine::TaskCancellationBlocker block_cancel;
    UASSERT(!response.IsSent());
    request.SetStartSendResponseTime();
    if (peer_socket_) {
      try {
        response.SendResponse(peer_socket_);
      } catch (const engine::io::IoSystemError& ex) {
        // working with raw values because std::errc compares error_category
        // default_error_category() fixed only in GCC 9.1 (PR libstdc++/60555)
        auto log_level =
            ex.Code().value() == static_cast<int>(std::errc::broken_pipe)
                ? logging::Level::kWarning
                : logging::Level::kError;
        LOG(log_level) << "I/O error while sending data: " << ex;
      } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while sending data: " << ex;

        send_failure_time_ = std::chrono::steady_clock::now();
        response.SetSendFailed(send_failure_time_);
      }
    } else {
      response.SetSendFailed(send_failure_time_);
    }
    request.SetFinishSendResponseTime();
    --stats_->active_request_count;
    ++stats_->requests_processed_count;

    request.WriteAccessLogs(request_handler_.LoggerAccess(),
                            request_handler_.LoggerAccessTskv(),
                            remote_address_);
  }

  LOG_TRACE() << "Stopping SendResponses()";

  CloseAsync();
}

void Connection::CloseAsync() {
  if (is_closing_.exchange(true)) return;

  // We should delete this, but we cannot do ~Connection() as it waits for
  // current task. So, create a mini-task for killing this.
  engine::impl::CriticalAsync(
      [](std::shared_ptr<Connection> shared_this) { shared_this.reset(); },
      std::move(shared_this_))
      .Detach();

  // 'this' might be deleted here
}

}  // namespace net
}  // namespace server
