#include <server/net/http2_connection.hpp>

#include <server/http/http2_session.hpp>
#include <server/http/http2_writer.hpp>
#include <server/http/http_request_parser.hpp>
#include <server/http/request_handler_base.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/request/request_config.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

namespace {

constexpr std::size_t kMinLenPrefaceToDetect = 3;

constexpr std::string_view kHttp2Preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
constexpr std::string_view kPrefaceBegin = kHttp2Preface.substr(0, kMinLenPrefaceToDetect);

constexpr std::uint64_t kSocketId = std::numeric_limits<std::uint64_t>::max();
constexpr std::uint64_t kStreamingId = std::numeric_limits<std::uint64_t>::max() - 1;

enum class WakeupKind { kSocketReadable, kStreamingReady, kTaskComputedResponse };

WakeupKind GetWakeupKind(std::uint64_t id) {
    if (id == kSocketId) {
        return WakeupKind::kSocketReadable;
    }
    if (id == kStreamingId) {
        return WakeupKind::kStreamingReady;
    }
    return WakeupKind::kTaskComputedResponse;
}

}  // namespace

Http2Connection::Http2Connection(
    const ConnectionConfig& config,
    const request::HttpRequestConfig& handler_defaults_config,
    std::unique_ptr<engine::io::RwBase> peer_socket,
    const engine::io::Sockaddr& remote_address,
    const http::RequestHandlerBase& request_handler,
    Stats& stats,
    request::ResponseDataAccounter& data_accounter
)
    : ConnectionBase(std::move(peer_socket), remote_address.PrimaryAddressString(), config, request_handler, stats),
      config_(config),
      handler_defaults_config_(handler_defaults_config),
      request_handler_(request_handler),
      stats_(stats),
      data_accounter_(data_accounter),
      remote_address_(remote_address)
{
    UASSERT(config_.http_version == USERVER_NAMESPACE::http::HttpVersion::k2);
    LOG_DEBUG() << "Incoming connection from " << GetPeerName() << ", fd " << GetFd();

    ++stats_.active_connections;
    ++stats_.connections_created;
}

Http2Connection::~Http2Connection() = default;

void Http2Connection::Process() {
    LOG_TRACE() << "Starting socket listener for fd " << GetFd();

    ListenForRequests();

    Shutdown();
}

std::shared_ptr<http::HttpRequest>& Http2Connection::GetFirstHttp1Request() {
    UINVARIANT(pending_requests_.size() == 1, "There must be single HTTP/1.1 request");
    auto& http1_request = pending_requests_.front();
    UINVARIANT(http1_request->GetHttpMajor() == 1, "It must be HTTP/1.1 request");
    return http1_request;
}

std::unique_ptr<engine::io::RwBase> Http2Connection::ExtractSocket() noexcept {
    return ConnectionBase::ExtractSocket();
}

void Http2Connection::ListenForRequests() {
    EnsureHttp2();
    UASSERT(!parser_);
    parser_ = MakeParser();
    handler_tasks_.reserve(config_.http2_session_config.max_concurrent_streams + 1);

    engine::WaitAnyContext wait_any{};
    wait_any.Append(kSocketId, GetSocket().GetReadableBase());
    wait_any.Append(kStreamingId, parser_->GetStreamingEvent());

    while (!engine::current_task::ShouldCancel()) {
        StartAllRequestTasks(wait_any);

        if (ShouldCloseConnection()) {
            return;
        }

        const auto ready_id = wait_any.WaitUntil(engine::Deadline::FromDuration(config_.keepalive_timeout));
        if (!ready_id) {
            if (ready_id == utils::unexpected(engine::WaitAnyError::kTimeout) && !IsIdle()) {
                continue;
            }
            return;
        }

        switch (GetWakeupKind(*ready_id)) {
            case WakeupKind::kSocketReadable:
                // `WaitReadable` in `TryParseRequests` doesn't block and just resets the "ready" flag
                // in the `Socket` read side so that the next `wait_any` usage doesn't return immediately.
                if (!TryParseRequests(*parser_)) {
                    return;
                }
                wait_any.Append(kSocketId, GetSocket().GetReadableBase());
                break;
            case WakeupKind::kStreamingReady:
                // The completed awaitable was dropped out of `wait_any`, so the
                // no-auto-reset event has no active awaiter and may be reset
                // here. Resetting before the drain keeps a signal arriving
                // mid-drain for the next round. `Reset()` is not allowed while
                // the event is appended (active awaiter), which is why the
                // drain in `OnRequestTaskFinished` leaves the signal alone.
                parser_->GetStreamingEvent().Reset();
                HandleStreamingEvents();
                wait_any.Append(kStreamingId, parser_->GetStreamingEvent());
                break;
            case WakeupKind::kTaskComputedResponse:
                OnRequestTaskFinished(*ready_id, wait_any);
                break;
        }

        UASSERT(wait_any.GetSize() <= config_.http2_session_config.max_concurrent_streams + 2);
    }
}

void Http2Connection::StartAllRequestTasks(engine::WaitAnyContext& wait_any) {
    for (auto& request : pending_requests_) {
        const auto& [handler, slot_id] = handler_tasks_.emplace(StartRequestTask(std::move(request)));
        wait_any.Append(slot_id, handler.task);
    }
    pending_requests_.clear();
}

Http2Connection::RequestTaskContext Http2Connection::StartRequestTask(std::shared_ptr<http::HttpRequest>&& request_ptr
) noexcept {
    if (request_ptr->IsFinal()) {
        StopAcceptingRequests();
    }

    stats_.active_request_count.Add(1);

    auto task = ConnectionBase::StartRequestTask(request_ptr);

    // `SetStreamBody()` is called synchronously in `StartRequestTask` before
    // the handler task is spawned, so `IsBodyStreamed()` is reliable here.
    // Requests without a stream id (h2c upgrade) keep the buffered send path.
    const auto& response = request_ptr->GetHttpResponse();
    if (response.IsBodyStreamed() && response.GetStreamId().has_value()) {
        streamed_requests_.emplace(*response.GetStreamId(), StreamedRequestContext{request_ptr, false});
    }

    return {.task = std::move(task), .request = std::move(request_ptr)};
}

void Http2Connection::OnRequestTaskFinished(std::uint64_t event_id, engine::WaitAnyContext& wait_any) noexcept {
    auto& task_context = handler_tasks_[event_id];
    if (task_context.is_upgraded) {
        FinishUpgradedStream(*task_context.request);
        handler_tasks_.erase(event_id);
        return;
    }

    auto& request = *task_context.request;
    const bool is_upgrade = request.IsUpgradeWebsocket();
    const auto stream_id = request.GetHttpResponse().GetStreamId();
    if (stream_id.has_value() && streamed_requests_.find(*stream_id) != streamed_requests_.end()) {
        // Drain the remaining body parts. `ResponseBodyStream` always pushes a
        // final event before the handler task completes, so this also submits
        // the response if no streaming event was processed for it yet.
        try {
            HandleStreamingEvents();
        } catch (const std::exception& ex) {
            LOG_ERROR() << "Error while sending streamed body parts: " << ex;
            request.GetHttpResponse().SetSendFailed(std::chrono::steady_clock::now());
        }
        SubmitStreamedResponseIfPending(*stream_id);
        FinalizeResponse(request);
        streamed_requests_.erase(*stream_id);
    } else {
        SendResponse(request);
    }

    auto request_ptr = std::move(task_context.request);
    handler_tasks_.erase(event_id);
    if (is_upgrade) {
        StartUpgradedTask(std::move(request_ptr), wait_any);
    }
}

void Http2Connection::StartUpgradedTask(HttpRequestPtr&& request_ptr, engine::WaitAnyContext& wait_any) noexcept {
    UASSERT(parser_);
    try {
        const auto stream_id = http::Stream::Id{request_ptr->GetHttpResponse().GetStreamId().value()};
        auto stream_rw = parser_->UpgradeStream(stream_id);
        // The tunnelled protocol runs in its own task, so that the connection keeps
        // multiplexing the other streams for as long as it lives.
        auto task = engine::CriticalAsyncNoTracing(
            [request = request_ptr, socket = std::move(stream_rw), peer_name = remote_address_]() mutable {
                request->DoUpgrade(std::move(socket), std::move(peer_name));
            }
        );
        const auto& [task_context, slot_id] = handler_tasks_.emplace(
            RequestTaskContext{.task = std::move(task), .request = std::move(request_ptr), .is_upgraded = true}
        );
        wait_any.Append(slot_id, task_context.task);
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Failed to upgrade a stream on fd " << GetFd() << ": " << ex;
    }
}

void Http2Connection::FinishUpgradedStream(const http::HttpRequest& request) noexcept {
    UASSERT(parser_);
    try {
        parser_->CloseUpgradedStream(http::Stream::Id{request.GetHttpResponse().GetStreamId().value()});
    } catch (const std::exception& ex) {
        LOG_WARNING() << "Failed to close an upgraded stream on fd " << GetFd() << ": " << ex;
    }
}

void Http2Connection::HandleStreamingEvents() {
    http::impl::Http2StreamEvent event;
    while (parser_->PopStreamingEventNoblock(event)) {
        // The first event for a stream means its headers are complete
        // (`SetHeadersEnd()` precedes the first `PushBodyChunk()`), so the
        // response with its deferred body provider is submitted here.
        SubmitStreamedResponseIfPending(event.stream_id);
        parser_->ApplyStreamingEvent(std::move(event));
        event = {};
    }
    parser_->WriteWhileWant();
}

void Http2Connection::SubmitStreamedResponseIfPending(std::int32_t stream_id) noexcept {
    const auto it = streamed_requests_.find(stream_id);
    if (it == streamed_requests_.end() || it->second.submit_attempted) {
        return;
    }
    it->second.submit_attempted = true;
    SubmitResponse(*it->second.request);
}

void Http2Connection::SendResponse(http::HttpRequest& request) noexcept {
    SubmitResponse(request);
    FinalizeResponse(request);
}

void Http2Connection::SubmitResponse(http::HttpRequest& request) noexcept {
    auto& response = request.GetHttpResponse();
    UASSERT(!response.IsSent());
    request.SetStartSendResponseTime();
    if (IsResponseChainValid()) {
        try {
            // Might be a stream reading or a fully constructed response
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            auto& http_response = static_cast<http::HttpResponse&>(response);
            if (const auto& h = request.GetHeader(USERVER_NAMESPACE::http::headers::k2::kHttp2SettingsHeader);
                !h.empty())
            {
                parser_->UpgradeToHttp2(h);
                http_response.SetStreamId(static_cast<std::int32_t>(http::kStreamIdAfterUpgradeResponse));
            }
            http::WriteHttp2ResponseToSocket(http_response, *parser_);
        } catch (const engine::io::IoSystemError& ex) {
            auto log_level = ex.Code().value() == EPIPE ? logging::Level::kWarning : logging::Level::kError;
            LOG(log_level) << "I/O error while sending data: " << ex;
            response.SetSendFailed(std::chrono::steady_clock::now());
        } catch (const std::exception& ex) {
            LOG_ERROR() << "Error while sending data: " << ex;
            response.SetSendFailed(std::chrono::steady_clock::now());
        }
    } else {
        response.SetSendFailed(std::chrono::steady_clock::now());
    }
}

void Http2Connection::FinalizeResponse(http::HttpRequest& request) noexcept {
    request.SetFinishSendResponseTime();
    stats_.active_request_count.Subtract(1);
    ++stats_.requests_processed_count;

    request.WriteAccessLogs(request_handler_.LoggerAccess(), request_handler_.LoggerAccessTskv(), GetPeerName());
}

bool Http2Connection::IsIdle() const noexcept { return handler_tasks_.empty(); }

bool Http2Connection::ShouldCloseConnection() const noexcept {
    UASSERT(parser_);
    // Check handler_tasks_ to do graceful shutdown.
    return !parser_->ConnectionIsOk() && handler_tasks_.empty();
}

std::unique_ptr<http::Http2Session> Http2Connection::MakeParser() {
    const auto on_req_cb = [this](HttpRequestPtr&& request_ptr) {
        pending_requests_.push_back(std::move(request_ptr));
    };
    return std::make_unique<http::Http2Session>(
        request_handler_.GetHandlerInfoIndex(),
        handler_defaults_config_,
        config_.http2_session_config,
        on_req_cb,
        stats_.parser_stats,
        data_accounter_,
        remote_address_,
        &GetSocket()
    );
}

void Http2Connection::EnsureHttp2() {
    // In HTTP/2.0 server we also want to support HTTP/1.0 to migrate clients smoothly.
    // There are 3 types of first request:
    // 1. Pure HTTP/2.0 (starts with preface).
    // 2. Pure HTTP/1.0.
    // 3. HTTP/1.0 with upgrade header.
    auto& reader = Reader();
    while (reader.BufferSize() < kMinLenPrefaceToDetect) {
        if (!reader.TryRead(GetSocket(), GetFd())) {
            throw std::runtime_error{fmt::format("Failed to read from socket {}. See logs.", GetFd())};
        }
    }
    if (reader.PeekAsStringView().substr(0, kMinLenPrefaceToDetect) == kPrefaceBegin) {
        // Case 1.
        return;
    }
    http::HttpRequestParser request_parser(
        request_handler_.GetHandlerInfoIndex(),
        handler_defaults_config_,
        [this](HttpRequestPtr&& request_ptr) {
            UINVARIANT(pending_requests_.empty(), "It must be the first request!");
            pending_requests_.push_back(std::move(request_ptr));
        },
        stats_.parser_stats,
        data_accounter_,
        remote_address_
    );
    while (pending_requests_.empty()) {
        if (reader.IsEmpty() && !reader.TryRead(GetSocket(), GetFd())) {
            throw std::runtime_error{fmt::format("Failed to read from socket {}. See logs.", GetFd())};
        }
        if (!request_parser.Parse(reader.PeekAsStringView())) {
            throw std::runtime_error{fmt::format("Malformed request from {} on fd {}.", GetPeerName(), GetFd())};
        }
        reader.ResetView();
    }
    if (!GetFirstHttp1Request()->GetHeader(USERVER_NAMESPACE::http::headers::k2::kHttp2SettingsHeader).empty()) {
        // Case 3.
        const auto send = GetSocket().WriteAll(
            http::kSwitchingProtocolResponse.data(),
            http::kSwitchingProtocolResponse.size(),
            engine::Deadline::FromDuration(config_.keepalive_timeout)
        );
        LOG_TRACE()
            << fmt::format("Written {} bytes, expected to write {}.", send, http::kSwitchingProtocolResponse.size());
        return;
    }
    // Case 2.
    throw Http1IsNotSupported{"HTTP/1.1 is not supported in Http2Connection"};
}

}  // namespace server::net

USERVER_NAMESPACE_END
