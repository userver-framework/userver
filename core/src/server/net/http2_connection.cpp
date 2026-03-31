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
    while (TryParseRequests(*parser_)) {
        for (auto&& request : pending_requests_) {
            ProcessRequest(std::move(request));
        }
        pending_requests_.clear();
    }
}

void Http2Connection::ProcessRequest(std::shared_ptr<http::HttpRequest>&& request_ptr) noexcept {
    if (request_ptr->IsFinal()) {
        StopAcceptingRequests();
    }

    stats_.active_request_count.Add(1);

    auto task = HandleQueueItem(request_ptr);
    SendResponse(*request_ptr);
}

void Http2Connection::SendResponse(http::HttpRequest& request) noexcept {
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
    request.SetFinishSendResponseTime();
    stats_.active_request_count.Subtract(1);
    ++stats_.requests_processed_count;

    request.WriteAccessLogs(request_handler_.LoggerAccess(), request_handler_.LoggerAccessTskv(), GetPeerName());
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
