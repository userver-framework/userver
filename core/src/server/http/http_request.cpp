#include <userver/server/http/http_request.hpp>

#include <server/handlers/http_handler_base_statistics.hpp>
#include <server/http/http_request_impl.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/parser/http_request_parse_args.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/logger.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/encoding/tskv.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::string EscapeLogString(std::string_view str, const std::array<uint8_t, 256>& need_escape_map) {
    size_t esc_cnt = 0;
    for (char ch : str) {
        if (need_escape_map[static_cast<uint8_t>(ch)]) esc_cnt++;
    }
    if (!esc_cnt) return std::string{str};
    std::string res;
    res.reserve(str.size() + esc_cnt * 3);
    for (char ch : str) {
        if (need_escape_map[static_cast<uint8_t>(ch)]) {
            res += '\\';
            res += 'x';
            res += ("0123456789ABCDEF"[(ch >> 4) & 0xF]);
            res += ("0123456789ABCDEF"[ch & 0xF]);
        } else {
            res += ch;
        }
    }
    return res;
}

std::string EscapeForAccessLog(std::string_view str) {
    constexpr auto prepare_need_escape = []() {
        std::array<uint8_t, 256> res{};
        for (int i = 0; i < 32; i++) res[i] = 1;
        for (int i = 127; i < 256; i++) res[i] = 1;
        res[static_cast<uint8_t>('\\')] = 1;
        res[static_cast<uint8_t>('"')] = 1;
        return res;
    };

    static constexpr auto kNeedEscape = prepare_need_escape();

    if (str.empty()) return "-";
    return EscapeLogString(str, kNeedEscape);
}

std::string EscapeForAccessTskvLog(std::string_view str) {
    if (str.empty()) return "-";

    std::string encoded_str;
    EncodeTskv(encoded_str, str, utils::encoding::EncodeTskvMode::kValue);
    return encoded_str;
}

const std::string kEmptyString{};
const std::vector<std::string> kEmptyVector{};

}  // namespace

namespace server::http {

HttpRequest::HttpRequest(request::ResponseDataAccounter& data_accounter, utils::impl::InternalTag)
    : pimpl_(*this, data_accounter) {}

HttpRequest::~HttpRequest() = default;

const HttpMethod& HttpRequest::GetMethod() const { return pimpl_->method_; }

const std::string& HttpRequest::GetMethodStr() const { return ToString(pimpl_->method_); }

int HttpRequest::GetHttpMajor() const { return pimpl_->http_major_; }

int HttpRequest::GetHttpMinor() const { return pimpl_->http_minor_; }

const std::string& HttpRequest::GetUrl() const { return pimpl_->url_; }

const std::string& HttpRequest::GetRequestPath() const { return pimpl_->request_path_; }

std::chrono::duration<double> HttpRequest::GetRequestTime() const {
    return GetHttpResponse().SentTime() - GetStartTime();
}

std::chrono::duration<double> HttpRequest::GetResponseTime() const {
    return GetHttpResponse().ReadyTime() - GetStartTime();
}

const std::string& HttpRequest::GetHost() const { return GetHeader(USERVER_NAMESPACE::http::headers::kHost); }

const engine::io::Sockaddr& HttpRequest::GetRemoteAddress() const { return pimpl_->remote_address_; }

const std::string& HttpRequest::GetArg(std::string_view arg_name) const {
#ifndef NDEBUG
    pimpl_->args_referenced_ = true;
#endif
    const auto* ptr = utils::impl::FindTransparentOrNullptr(pimpl_->request_args_, arg_name);
    if (!ptr) return kEmptyString;
    return ptr->at(0);
}

const std::vector<std::string>& HttpRequest::GetArgVector(std::string_view arg_name) const {
#ifndef NDEBUG
    pimpl_->args_referenced_ = true;
#endif
    const auto* ptr = utils::impl::FindTransparentOrNullptr(pimpl_->request_args_, arg_name);
    if (!ptr) return kEmptyVector;
    return *ptr;
}

bool HttpRequest::HasArg(std::string_view arg_name) const {
    const auto* ptr = utils::impl::FindTransparentOrNullptr(pimpl_->request_args_, arg_name);
    return !!ptr;
}

size_t HttpRequest::ArgCount() const { return pimpl_->request_args_.size(); }

std::vector<std::string> HttpRequest::ArgNames() const {
    std::vector<std::string> res;
    res.reserve(pimpl_->request_args_.size());
    for (const auto& arg : pimpl_->request_args_) res.push_back(arg.first);
    return res;
}

const FormDataArg& HttpRequest::GetFormDataArg(std::string_view arg_name) const {
    static const FormDataArg kEmptyFormDataArg{};

    const auto* ptr = utils::impl::FindTransparentOrNullptr(pimpl_->form_data_args_, arg_name);
    if (!ptr) return kEmptyFormDataArg;
    return ptr->at(0);
}

const std::vector<FormDataArg>& HttpRequest::GetFormDataArgVector(std::string_view arg_name) const {
    static const std::vector<FormDataArg> kEmptyFormDataArgVector{};

    const auto* ptr = utils::impl::FindTransparentOrNullptr(pimpl_->form_data_args_, arg_name);
    if (!ptr) return kEmptyFormDataArgVector;
    return *ptr;
}

bool HttpRequest::HasFormDataArg(std::string_view arg_name) const {
    const auto* ptr = utils::impl::FindTransparentOrNullptr(pimpl_->form_data_args_, arg_name);
    return !!ptr;
}

size_t HttpRequest::FormDataArgCount() const { return pimpl_->form_data_args_.size(); }

std::vector<std::string> HttpRequest::FormDataArgNames() const {
    std::vector<std::string> res;
    res.reserve(pimpl_->form_data_args_.size());
    for (const auto& [name, _] : pimpl_->form_data_args_) res.push_back(name);
    return res;
}

const std::string& HttpRequest::GetPathArg(std::string_view arg_name) const {
    const auto* ptr = utils::impl::FindTransparentOrNullptr(pimpl_->path_args_by_name_index_, arg_name);
    if (!ptr) return kEmptyString;
    UASSERT(*ptr < pimpl_->path_args_.size());
    return pimpl_->path_args_[*ptr];
}

const std::string& HttpRequest::GetPathArg(size_t index) const {
    return index < PathArgCount() ? pimpl_->path_args_[index] : kEmptyString;
}

bool HttpRequest::HasPathArg(std::string_view arg_name) const {
    return !!utils::impl::FindTransparentOrNullptr(pimpl_->path_args_by_name_index_, arg_name);
}

bool HttpRequest::HasPathArg(size_t index) const { return index < PathArgCount(); }

size_t HttpRequest::PathArgCount() const { return pimpl_->path_args_.size(); }

const std::string& HttpRequest::GetHeader(std::string_view header_name) const {
    auto it = pimpl_->headers_.find(header_name);
    if (it == pimpl_->headers_.end()) return kEmptyString;
    return it->second;
}

const std::string& HttpRequest::GetHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) const {
    auto it = pimpl_->headers_.find(header_name);
    if (it == pimpl_->headers_.end()) return kEmptyString;
    return it->second;
}

bool HttpRequest::HasHeader(std::string_view header_name) const { return pimpl_->headers_.count(header_name) != 0; }

bool HttpRequest::HasHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) const {
    return pimpl_->headers_.count(header_name) != 0;
}

size_t HttpRequest::HeaderCount() const { return pimpl_->headers_.size(); }

void HttpRequest::RemoveHeader(std::string_view header_name) { pimpl_->headers_.erase(header_name); }

void HttpRequest::RemoveHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) {
    pimpl_->headers_.erase(header_name);
}

HttpRequest::HeadersMapKeys HttpRequest::GetHeaderNames() const {
    return HttpRequest::HeadersMapKeys{pimpl_->headers_};
}

const HttpRequest::HeadersMap& HttpRequest::GetHeaders() const { return pimpl_->headers_; }

const std::string& HttpRequest::GetCookie(const std::string& cookie_name) const {
    auto it = pimpl_->cookies_.find(cookie_name);
    if (it == pimpl_->cookies_.end()) return kEmptyString;
    return it->second;
}

bool HttpRequest::HasCookie(const std::string& cookie_name) const { return pimpl_->cookies_.count(cookie_name); }

size_t HttpRequest::CookieCount() const { return pimpl_->cookies_.size(); }

HttpRequest::CookiesMapKeys HttpRequest::GetCookieNames() const {
    return HttpRequest::CookiesMapKeys{pimpl_->cookies_};
}

const HttpRequest::CookiesMap& HttpRequest::RequestCookies() const { return pimpl_->cookies_; }

const std::string& HttpRequest::RequestBody() const { return pimpl_->request_body_; }

std::string HttpRequest::ExtractRequestBody() { return std::move(pimpl_->request_body_); }

void HttpRequest::SetRequestBody(std::string body) { pimpl_->request_body_ = std::move(body); }

void HttpRequest::ParseArgsFromBody() {
#ifndef NDEBUG
    UASSERT_MSG(
        !pimpl_->args_referenced_,
        "References to arguments could be invalidated by ParseArgsFromBody(). "
        "Avoid calling GetArg()/GetArgVector() before ParseArgsFromBody()"
    );
#endif

    USERVER_NAMESPACE::http::parser::ParseAndConsumeArgs(
        pimpl_->request_body_,
        [this](std::string&& key, std::string&& value) {
            pimpl_->request_args_[std::move(key)].push_back(std::move(value));
        }
    );
}

bool HttpRequest::IsFinal() const { return pimpl_->is_final_; }

void HttpRequest::SetResponseStatus(HttpStatus status) const { pimpl_->response_.SetStatus(status); }

bool HttpRequest::IsBodyCompressed() const {
    const auto& encoding = GetHeader(USERVER_NAMESPACE::http::headers::kContentEncoding);
    return !encoding.empty() && encoding != "identity";
}

HttpResponse& HttpRequest::GetHttpResponse() const { return pimpl_->response_; }

std::chrono::steady_clock::time_point HttpRequest::GetStartTime() const { return pimpl_->start_time_; }

bool HttpRequest::IsUpgradeWebsocket() const { return static_cast<bool>(pimpl_->upgrade_websocket_cb_); }

void HttpRequest::SetUpgradeWebsocket(UpgradeCallback cb) const { pimpl_->upgrade_websocket_cb_ = std::move(cb); }

void HttpRequest::DoUpgrade(std::unique_ptr<engine::io::RwBase>&& socket, engine::io::Sockaddr&& peer_name) const {
    pimpl_->upgrade_websocket_cb_(std::move(socket), std::move(peer_name));
}

void HttpRequest::SetPathArgs(std::vector<std::pair<std::string, std::string>> args) {
    pimpl_->path_args_.clear();
    pimpl_->path_args_.reserve(args.size());

    pimpl_->path_args_by_name_index_.clear();
    for (auto& [name, value] : args) {
        pimpl_->path_args_.push_back(std::move(value));
        if (!name.empty()) {
            pimpl_->path_args_by_name_index_[std::move(name)] = pimpl_->path_args_.size() - 1;
        }
    }
}

void HttpRequest::AccountResponseTime() {
    if (pimpl_->request_statistics_) {
        auto timing = std::chrono::duration_cast<std::chrono::milliseconds>(
            pimpl_->finish_send_response_time_ - pimpl_->start_time_
        );
        pimpl_->request_statistics_->ForMethod(GetMethod()).Account(handlers::HttpRequestStatisticsEntry{timing});
    }
}

void HttpRequest::MarkAsInternalServerError() const {
    // TODO : refactor, this being here is a bit ridiculous
    pimpl_->response_.SetStatus(http::HttpStatus::kInternalServerError);
    pimpl_->response_.SetData({});
    pimpl_->response_.ClearHeaders();
}

void HttpRequest::SetHttpHandler(const handlers::HttpHandlerBase& handler) { pimpl_->handler_ = &handler; }

const handlers::HttpHandlerBase* HttpRequest::GetHttpHandler() const { return pimpl_->handler_; }

void HttpRequest::SetTaskProcessor(engine::TaskProcessor& task_processor) { pimpl_->task_processor_ = &task_processor; }

engine::TaskProcessor* HttpRequest::GetTaskProcessor() const { return pimpl_->task_processor_; }

void HttpRequest::SetHttpHandlerStatistics(handlers::HttpRequestStatistics& stats) {
    pimpl_->request_statistics_ = &stats;
}

void HttpRequest::SetResponseStreamId(std::int32_t stream_id) { pimpl_->response_.SetStreamId(stream_id); }

void HttpRequest::SetStreamProducer(impl::Http2StreamEventProducer&& producer) {
    pimpl_->response_.SetStreamProdicer(std::move(producer));
}

void HttpRequest::SetTaskCreateTime() { pimpl_->task_create_time_ = std::chrono::steady_clock::now(); }

void HttpRequest::SetTaskStartTime() { pimpl_->task_start_time_ = std::chrono::steady_clock::now(); }

void HttpRequest::SetResponseNotifyTime() { SetResponseNotifyTime(std::chrono::steady_clock::now()); }

void HttpRequest::SetResponseNotifyTime(std::chrono::steady_clock::time_point now) {
    pimpl_->response_notify_time_ = now;
}

void HttpRequest::SetStartSendResponseTime() { pimpl_->start_send_response_time_ = std::chrono::steady_clock::now(); }

void HttpRequest::SetFinishSendResponseTime() {
    pimpl_->finish_send_response_time_ = std::chrono::steady_clock::now();
    AccountResponseTime();
}

void HttpRequest::WriteAccessLogs(
    const logging::TextLoggerPtr& logger_access,
    const logging::TextLoggerPtr& logger_access_tskv,
    const std::string& remote_address
) const {
    if (!logger_access && !logger_access_tskv) return;

    const auto tp = utils::datetime::WallCoarseClock::now();
    WriteAccessLog(logger_access, tp, remote_address);
    WriteAccessTskvLog(logger_access_tskv, tp, remote_address);
}

void HttpRequest::WriteAccessLog(
    const logging::TextLoggerPtr& logger_access,
    utils::datetime::WallCoarseClock::time_point tp,
    const std::string& remote_address
) const {
    if (!logger_access) return;

    logging::impl::TextLogItem item{
        fmt::format(
            R"([{}] {} {} "{} {} HTTP/{}.{}" {} "{}" "{}" "{}" {:0.6f} - {} {:0.6f})",
            utils::datetime::LocalTimezoneTimestring(tp, "%Y-%m-%d %H:%M:%E6S %Ez"),
            EscapeForAccessLog(GetHost()),
            EscapeForAccessLog(remote_address),
            EscapeForAccessLog(GetMethodStr()),
            EscapeForAccessLog(GetUrl()),
            GetHttpMajor(),
            GetHttpMinor(),
            static_cast<int>(pimpl_->response_.GetStatus()),
            EscapeForAccessLog(GetHeader("Referer")),
            EscapeForAccessLog(GetHeader("User-Agent")),
            EscapeForAccessLog(GetHeader("Cookie")),
            GetRequestTime().count(),
            GetHttpResponse().BytesSent(),
            GetResponseTime().count()
        ),
    };
    logger_access->Log(logging::Level::kInfo, item);
}

void HttpRequest::WriteAccessTskvLog(
    const logging::TextLoggerPtr& logger_access_tskv,
    utils::datetime::WallCoarseClock::time_point tp,
    const std::string& remote_address
) const {
    if (!logger_access_tskv) return;

    logging::impl::TextLogItem item{fmt::format(
        "tskv"
        "\t{}"
        "\tstatus={}"
        "\tprotocol=HTTP/{}.{}"
        "\tmethod={}"
        "\trequest={}"
        "\treferer={}"
        "\tcookies={}"
        "\tuser_agent={}"
        "\tvhost={}"
        "\tip={}"
        "\tx_forwarded_for={}"
        "\tx_real_ip={}"
        "\tupstream_http_x_yarequestid={}"
        "\thttp_host={}"
        "\tremote_addr={}"
        "\trequest_time={:0.3f}"
        "\tupstream_response_time={:0.3f}"
        "\trequest_body={}",
        utils::datetime::LocalTimezoneTimestring(tp, "timestamp=%Y-%m-%dT%H:%M:%S\ttimezone=%Ez"),
        static_cast<int>(pimpl_->response_.GetStatus()),
        GetHttpMajor(),
        GetHttpMinor(),
        EscapeForAccessTskvLog(GetMethodStr()),
        EscapeForAccessTskvLog(GetUrl()),
        EscapeForAccessTskvLog(GetHeader("Referer")),
        EscapeForAccessTskvLog(GetHeader("Cookie")),
        EscapeForAccessTskvLog(GetHeader("User-Agent")),
        EscapeForAccessTskvLog(GetHost()),
        EscapeForAccessTskvLog(remote_address),
        EscapeForAccessTskvLog(GetHeader("X-Forwarded-For")),
        EscapeForAccessTskvLog(GetHeader("X-Real-IP")),
        EscapeForAccessTskvLog(GetHeader("X-YaRequestId")),
        EscapeForAccessTskvLog(GetHost()),
        EscapeForAccessTskvLog(remote_address),
        GetRequestTime().count(),
        GetResponseTime().count(),
        EscapeForAccessTskvLog(RequestBody())
    )};
    logger_access_tskv->Log(logging::Level::kInfo, item);
}

}  // namespace server::http

USERVER_NAMESPACE_END
