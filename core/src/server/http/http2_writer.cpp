#include <server/http/http2_writer.hpp>

#include <fmt/format.h>

#include <server/http/http2_session.hpp>
#include <server/http/http_cached_date.hpp>
#include <server/http/http_request_parser.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/predefined_header.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/utils/impl/projecting_view.hpp>
#include <userver/utils/small_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

constexpr std::string_view kDefaultContentTypeString = "application/json";

bool IsBodyForbiddenForStatus(HttpStatus status) {
  return status == HttpStatus::kNoContent ||
         status == HttpStatus::kNotModified ||
         (static_cast<int>(status) >= 100 && static_cast<int>(status) < 200);
}

ssize_t Nghttp2SendString(nghttp2_session*, int32_t, uint8_t*, std::size_t,
                          uint32_t*, nghttp2_data_source*, void*);

struct DataBufferSender final {
  DataBufferSender(std::string_view data)
      : data{reinterpret_cast<const uint8_t*>(data.data())}, size{data.size()} {
    nghttp2_provider.read_callback = Nghttp2SendString;
    nghttp2_provider.source.ptr = this;
  }

  nghttp2_data_provider nghttp2_provider{};

  const uint8_t* data{nullptr};
  const std::size_t size{0};
  std::size_t pos{0};
};

// implements
// https://nghttp2.org/documentation/types.html#c.nghttp2_data_source_read_callback
ssize_t Nghttp2SendString(nghttp2_session*, int32_t, uint8_t* p,
                          std::size_t want, uint32_t* flags,
                          nghttp2_data_source* source, void*) {
  auto& data_to_send = *static_cast<DataBufferSender*>(source->ptr);

  const std::size_t remaining = data_to_send.size - data_to_send.pos;

  // TODO: handle NGHTTP2_DATA_FLAG_NO_COPY and use OnSend callback
  if (remaining > want) {
    std::memcpy(p, data_to_send.data + data_to_send.pos, want);
    data_to_send.pos += want;
    return want;
  }
  std::memcpy(p, data_to_send.data + data_to_send.pos, remaining);
  *flags = NGHTTP2_DATA_FLAG_NONE;
  *flags |= NGHTTP2_DATA_FLAG_EOF;
  return remaining;
}

nghttp2_nv UnsafeHeaderToNGHeader(std::string_view name, std::string_view value,
                                  const bool sensitive) {
  nghttp2_nv result;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  result.name = reinterpret_cast<uint8_t*>(const_cast<char*>(name.data()));

  result.namelen = name.size();

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast),
  result.value = reinterpret_cast<uint8_t*>(const_cast<char*>(value.data()));
  result.valuelen = value.size();
  // no_copy_name -- we must to lower case all headers
  // no_copy_value -- we must to store all values until
  // nghttp2_on_frame_send_callback or nghttp2_on_frame_not_send_callback not
  // called result.flags = NGHTTP2_NV_FLAG_NO_COPY_VALUE |
  // NGHTTP2_NV_FLAG_NO_COPY_NAME;
  result.flags = NGHTTP2_NV_FLAG_NONE;
  if (sensitive) {
    result.flags |= NGHTTP2_NV_FLAG_NO_INDEX;
  }

  return result;
}

}  // namespace

class Http2HeaderWriter final {
 public:
  // We must borrow key-value pairs until the `SendResponseToSocket` is called
  explicit Http2HeaderWriter(std::size_t nheaders) {
    values_.reserve(nheaders);
  }

  void AddKeyValue(std::string_view key, std::string&& value) {
    const auto* ptr = values_.data();
    values_.push_back(std::move(value));
    UASSERT(ptr == values_.data());
    ng_headers_.push_back(UnsafeHeaderToNGHeader(key, values_.back(), false));
  }

  void AddKeyValue(std::string_view key, std::string_view value) {
    ng_headers_.push_back(UnsafeHeaderToNGHeader(key, value, false));
  }

  void AddCookie(const Cookie& cookie) {
    USERVER_NAMESPACE::http::headers::HeadersString val;
    cookie.AppendToString(val);
    const auto* ptr = values_.data();
    // TODO: avoid copy
    values_.push_back(std::string{val.data(), val.size()});
    UASSERT(ptr == values_.data());
    ng_headers_.push_back(UnsafeHeaderToNGHeader(
        USERVER_NAMESPACE::http::headers::kSetCookie, values_.back(), false));
  }

  std::vector<nghttp2_nv>& GetNgHeaders() { return ng_headers_; }

 private:
  std::vector<std::string> values_;
  std::vector<nghttp2_nv> ng_headers_;
};

class Http2ResponseWriter final {
 public:
  Http2ResponseWriter(HttpResponse& response, nghttp2_session* session)
      : response_(response), session_{session} {}

  void WriteHttpResponse(engine::io::RwBase& socket) {
    auto headers = WriteHeaders();
    if (response_.IsBodyStreamed() && response_.GetData().empty()) {
      WriteHttp2BodyStreamed(socket, headers);
    } else {
      // e.g. a CustomHandlerException
      WriteHttp2BodyNotstreamed(socket, headers);
    }
  }

 private:
  Http2HeaderWriter WriteHeaders() {
    // Preallocate space for all headers
    Http2HeaderWriter header_writer{response_.headers_.size() +
                                    response_.cookies_.size() + 3};

    header_writer.AddKeyValue(
        USERVER_NAMESPACE::http::headers::k2::kStatus,
        fmt::to_string(static_cast<std::uint16_t>(response_.status_)));

    const auto& headers = response_.headers_;
    const auto end = headers.cend();
    if (headers.find(USERVER_NAMESPACE::http::headers::kDate) == end) {
      header_writer.AddKeyValue(USERVER_NAMESPACE::http::headers::kDate,
                                std::string{impl::GetCachedDate()});
    }
    if (headers.find(USERVER_NAMESPACE::http::headers::kContentType) == end) {
      header_writer.AddKeyValue(USERVER_NAMESPACE::http::headers::kContentType,
                                kDefaultContentTypeString);
    }
    for (const auto& [key, value] : headers) {
      if (key ==
          std::string{USERVER_NAMESPACE::http::headers::kContentLength}) {
        continue;
      }
      header_writer.AddKeyValue(key, value);
    }
    for (const auto& value :
         USERVER_NAMESPACE::utils::impl::MakeValuesView(response_.cookies_)) {
      header_writer.AddCookie(value);
    }
    return header_writer;
  }

  std::size_t SendResponseToSocket(engine::io::RwBase& socket) {
    USERVER_NAMESPACE::http::headers::HeadersString res{};
    while (nghttp2_session_want_write(session_)) {
      while (true) {
        const std::uint8_t* data_ptr = nullptr;
        const std::size_t data_length =
            nghttp2_session_mem_send(session_, &data_ptr);
        if (data_length <= 0) {
          break;
        }
        const std::string_view a{reinterpret_cast<const char*>(data_ptr),
                                 data_length};
        res.append(a);
      }
    }
    return socket.WriteAll(res.data(), res.size(), {});
  }

  void WriteHttp2BodyNotstreamed(engine::io::RwBase& socket,
                                 Http2HeaderWriter& header_writer) {
    const bool is_body_forbidden = IsBodyForbiddenForStatus(response_.status_);
    const bool is_head_request =
        response_.request_.GetMethod() == HttpMethod::kHead;
    const auto& data = response_.GetData();

    if (!is_body_forbidden) {
      header_writer.AddKeyValue(
          USERVER_NAMESPACE::http::headers::kContentLength,
          fmt::to_string(data.size()));
    }

    if (is_body_forbidden && !data.empty()) {
      LOG_LIMITED_WARNING()
          << "Non-empty body provided for response with HTTP2 code "
          << static_cast<int>(response_.status_)
          << " which does not allow one, it will be dropped";
    }

    DataBufferSender data_sender{data};
    nghttp2_data_provider* provider{nullptr};
    if (!is_head_request && !is_body_forbidden) {
      provider = &data_sender.nghttp2_provider;
    }
    const std::uint32_t stream_id = response_.GetStreamId().value();
    const auto& nva = header_writer.GetNgHeaders();
    const int rv = nghttp2_submit_response(session_, stream_id, nva.data(),
                                           nva.size(), provider);
    if (rv != 0) {
      throw std::runtime_error{
          fmt::format("Fail to submit the response with err id = {}. Err: {}",
                      rv, nghttp2_strerror(rv))};
    }

    const auto sent_bytes = SendResponseToSocket(socket);
    response_.SetSent(sent_bytes, std::chrono::steady_clock::now());
  }

  void WriteHttp2BodyStreamed(engine::io::RwBase&, Http2HeaderWriter&) {
    UINVARIANT(false, "Not implemented for HTTP2.0");
  }

  HttpResponse& response_;
  nghttp2_session* session_;
};

void WriteHttp2ResponseToSocket(engine::io::RwBase& socket,
                                HttpResponse& response, Http2Session& parser) {
  Http2ResponseWriter w{response, parser.GetNghttp2SessionPtr()};
  w.WriteHttpResponse(socket);
}

}  // namespace server::http

USERVER_NAMESPACE_END
