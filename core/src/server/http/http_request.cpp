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
    for (const char ch : str) {
        if (need_escape_map[static_cast<uint8_t>(ch)]) esc_cnt++;
    }
    if (!esc_cnt) return std::string{str};
    std::string res;
    res.reserve(str.size() + esc_cnt * 3);
    for (const char ch : str) {
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

const HttpMethod& HttpRequest::GetMethod() const { return pimpl_->method; }

const std::string& HttpRequest::GetMethodStr() const { return ToString(pimpl_->method); }

int HttpRequest::GetHttpMajor() const { return pimpl_->http_major; }

int HttpRequest::GetHttpMinor() const { return pimpl_->http_minor; }

const std::string& HttpRequest::GetUrl() const { return pimpl_->url; }

const std::string& HttpRequest::GetRequestPath() const { return pimpl_->request_path; }

std::chrono::duration<double> HttpRequest::GetRequestTime() const {
    return GetHttpResponse().SentTime() - GetStartTime();
}

std::chrono::duration<double> HttpRequest::GetResponseTime() const {
    return GetHttpResponse().ReadyTime() - GetStartTime();
}

const std::string& HttpRequest::GetHost() const { return GetHeader(USERVER_NAMESPACE::http::headers::kHost); }

const engine::io::Sockaddr& HttpRequest::GetRemoteAddress() const { return pimpl_->remote_address; }

const std::string& HttpRequest::GetArg(std::string_view arg_name) const {
#ifndef NDEBUG
    pimpl_->args_referenced = true;
#endif
    const auto* ptr = utils::impl::FindTransparentOrNullptr(pimpl_->request_args, arg_name);
    if (!ptr) return kEmptyString;
    return ptr->at(0);
}

const std::vector<std::string>& HttpRequest::GetArgVector(std::string_view arg_name) const {
#ifndef NDEBUG
    pimpl_->args_referenced = true;
#endif
    const auto* ptr = utils::impl::FindTransparentOrNullptr(pimpl_->request_args, arg_name);
    if (!ptr) return kEmptyVector;
    return *ptr;
}

bool HttpRequest::HasArg(std::string_view arg_name) const {
    const auto* ptr = utils::impl::FindTransparentOrNullptr(pimpl_->request_args, arg_name);
    return !!ptr;
}

size_t HttpRequest::ArgCount() const { return pimpl_->request_args.size(); }

std::vector<std::string> HttpRequest::ArgNames() const {
    std::vector<std::string> res;
    res.reserve(pimpl_->request_args.size());
    for (const auto& arg : pimpl_->request_args) res.push_back(arg.first);
    return res;
}

const FormDataArg& HttpRequest::GetFormDataArg(std::string_view arg_name) const {
    static const FormDataArg kEmptyFormDataArg{};

    const auto* ptr = utils::impl::FindTransparentOrNullptr(pimpl_->form_data_args, arg_name);
    if (!ptr) return kEmptyFormDataArg;
    return ptr->at(0);
}

const std::vector<FormDataArg>& HttpRequest::GetFormDataArgVector(std::string_view arg_name) const {
    static const std::vector<FormDataArg> kEmptyFormDataArgVector{};

    const auto* ptr = utils::impl::FindTransparentOrNullptr(pimpl_->form_data_args, arg_name);
    if (!ptr) return kEmptyFormDataArgVector;
    return *ptr;
}

bool HttpRequest::HasFormDataArg(std::string_view arg_name) const {
    const auto* ptr = utils::impl::FindTransparentOrNullptr(pimpl_->form_data_args, arg_name);
    return !!ptr;
}

size_t HttpRequest::FormDataArgCount() const { return pimpl_->form_data_args.size(); }

std::vector<std::string> HttpRequest::FormDataArgNames() const {
    std::vector<std::string> res;
    res.reserve(pimpl_->form_data_args.size());
    for (const auto& [name, _] : pimpl_->form_data_args) res.push_back(name);
    return res;
}

const std::string& HttpRequest::GetPathArg(std::string_view arg_name) const {
    const auto* ptr = utils::impl::FindTransparentOrNullptr(pimpl_->path_args_by_name_index, arg_name);
    if (!ptr) return kEmptyString;
    UASSERT(*ptr < pimpl_->path_args.size());
    return pimpl_->path_args[*ptr];
}

const std::string& HttpRequest::GetPathArg(size_t index) const {
    return index < PathArgCount() ? pimpl_->path_args[index] : kEmptyString;
}

bool HttpRequest::HasPathArg(std::string_view arg_name) const {
    return !!utils::impl::FindTransparentOrNullptr(pimpl_->path_args_by_name_index, arg_name);
}

bool HttpRequest::HasPathArg(size_t index) const { return index < PathArgCount(); }

size_t HttpRequest::PathArgCount() const { return pimpl_->path_args.size(); }

const std::string& HttpRequest::GetHeader(std::string_view header_name) const {
    auto it = pimpl_->headers.find(header_name);
    if (it == pimpl_->headers.end()) return kEmptyString;
    return it->second;
}

const std::string& HttpRequest::GetHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) const {
    auto it = pimpl_->headers.find(header_name);
    if (it == pimpl_->headers.end()) return kEmptyString;
    return it->second;
}

bool HttpRequest::HasHeader(std::string_view header_name) const { return pimpl_->headers.count(header_name) != 0; }

bool HttpRequest::HasHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) const {
    return pimpl_->headers.count(header_name) != 0;
}

size_t HttpRequest::HeaderCount() const { return pimpl_->headers.size(); }

void HttpRequest::RemoveHeader(std::string_view header_name) { pimpl_->headers.erase(header_name); }

void HttpRequest::RemoveHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) {
    pimpl_->headers.erase(header_name);
}

HttpRequest::HeadersMapKeys HttpRequest::GetHeaderNames() const { return HttpRequest::HeadersMapKeys{pimpl_->headers}; }

const HttpRequest::HeadersMap& HttpRequest::GetHeaders() const { return pimpl_->headers; }

const std::string& HttpRequest::GetCookie(const std::string& cookie_name) const {
    auto it = pimpl_->cookies.find(cookie_name);
    if (it == pimpl_->cookies.end()) return kEmptyString;
    return it->second;
}

bool HttpRequest::HasCookie(const std::string& cookie_name) const { return pimpl_->cookies.count(cookie_name); }

size_t HttpRequest::CookieCount() const { return pimpl_->cookies.size(); }

HttpRequest::CookiesMapKeys HttpRequest::GetCookieNames() const { return HttpRequest::CookiesMapKeys{pimpl_->cookies}; }

const HttpRequest::CookiesMap& HttpRequest::RequestCookies() const { return pimpl_->cookies; }

const std::string& HttpRequest::RequestBody() const { return pimpl_->request_body; }

std::string HttpRequest::ExtractRequestBody() { return std::move(pimpl_->request_body); }

void HttpRequest::SetRequestBody(std::string body) { pimpl_->request_body = std::move(body); }

void HttpRequest::ParseArgsFromBody() {
#ifndef NDEBUG
    UASSERT_MSG(
        !pimpl_->args_referenced,
        "References to arguments could be invalidated by ParseArgsFromBody(). "
        "Avoid calling GetArg()/GetArgVector() before ParseArgsFromBody()"
    );
#endif

    USERVER_NAMESPACE::http::parser::ParseAndConsumeArgs(
        pimpl_->request_body,
        [this](std::string&& key, std::string&& value) {
            pimpl_->request_args[std::move(key)].push_back(std::move(value));
        }
    );
}

bool HttpRequest::IsFinal() const { return pimpl_->is_final; }

void HttpRequest::SetResponseStatus(HttpStatus status) const { pimpl_->response.SetStatus(status); }

bool HttpRequest::IsBodyCompressed() const {
    const auto& encoding = GetHeader(USERVER_NAMESPACE::http::headers::kContentEncoding);
    return !encoding.empty() && encoding != "identity";
}

HttpResponse& HttpRequest::GetHttpResponse() const { return pimpl_->response; }

std::chrono::steady_clock::time_point HttpRequest::GetStartTime() const { return pimpl_->start_time; }

bool HttpRequest::IsUpgradeWebsocket() const { return static_cast<bool>(pimpl_->upgrade_websocket_cb); }

void HttpRequest::SetUpgradeWebsocket(UpgradeCallback cb) const { pimpl_->upgrade_websocket_cb = std::move(cb); }

void HttpRequest::DoUpgrade(std::unique_ptr<engine::io::RwBase>&& socket, engine::io::Sockaddr&& peer_name) const {
    pimpl_->upgrade_websocket_cb(std::move(socket), std::move(peer_name));
}

void HttpRequest::SetPathArgs(std::vector<std::pair<std::string, std::string>> args) {
    pimpl_->path_args.clear();
    pimpl_->path_args.reserve(args.size());

    pimpl_->path_args_by_name_index.clear();
    for (auto& [name, value] : args) {
        pimpl_->path_args.push_back(std::move(value));
        if (!name.empty()) {
            pimpl_->path_args_by_name_index[std::move(name)] = pimpl_->path_args.size() - 1;
        }
    }
}

void HttpRequest::AccountResponseTime() {
    if (pimpl_->request_statistics) {
        auto timing = std::chrono::duration_cast<std::chrono::milliseconds>(
            pimpl_->finish_send_response_time - pimpl_->start_time
        );
        pimpl_->request_statistics->ForMethod(GetMethod()).Account(handlers::HttpRequestStatisticsEntry{timing});
    }
}

void HttpRequest::MarkAsInternalServerError() const {
    // TODO : refactor, this being here is a bit ridiculous
    pimpl_->response.SetStatus(http::HttpStatus::kInternalServerError);
    pimpl_->response.SetData({});

    std::string server_header = pimpl_->response.GetHeader(USERVER_NAMESPACE::http::headers::kServer);
    pimpl_->response.ClearHeaders();
    if (!server_header.empty()) {
        pimpl_->response.SetHeader(USERVER_NAMESPACE::http::headers::kServer, std::move(server_header));
    }
}

void HttpRequest::SetHttpHandler(const handlers::HttpHandlerBase& handler) { pimpl_->handler = &handler; }

const handlers::HttpHandlerBase* HttpRequest::GetHttpHandler() const { return pimpl_->handler; }

void HttpRequest::SetTaskProcessor(engine::TaskProcessor& task_processor) { pimpl_->task_processor = &task_processor; }

engine::TaskProcessor* HttpRequest::GetTaskProcessor() const { return pimpl_->task_processor; }

void HttpRequest::SetHttpHandlerStatistics(handlers::HttpRequestStatistics& stats) {
    pimpl_->request_statistics = &stats;
}

void HttpRequest::SetResponseStreamId(std::int32_t stream_id) { pimpl_->response.SetStreamId(stream_id); }

void HttpRequest::SetStreamProducer(impl::Http2StreamEventProducer&& producer) {
    pimpl_->response.SetStreamProdicer(std::move(producer));
}

void HttpRequest::SetTaskCreateTime() { pimpl_->task_create_time = std::chrono::steady_clock::now(); }

void HttpRequest::SetTaskStartTime() { pimpl_->task_start_time = std::chrono::steady_clock::now(); }

void HttpRequest::SetResponseNotifyTime() { SetResponseNotifyTime(std::chrono::steady_clock::now()); }

void HttpRequest::SetResponseNotifyTime(std::chrono::steady_clock::time_point now) {
    pimpl_->response_notify_time = now;
}

void HttpRequest::SetStartSendResponseTime() { pimpl_->start_send_response_time = std::chrono::steady_clock::now(); }

void HttpRequest::SetFinishSendResponseTime() {
    pimpl_->finish_send_response_time = std::chrono::steady_clock::now();
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
            static_cast<int>(pimpl_->response.GetStatus()),
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
        static_cast<int>(pimpl_->response.GetStatus()),
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
