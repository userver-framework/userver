#include <server/http/http_writer.hpp>

#include <fmt/format.h>

#include <server/http/http2_request_parser.hpp>
#include <server/http/http_cached_date.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/predefined_header.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_response.hpp>
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

ssize_t Nghttp2SendString(nghttp2_session*, int32_t, uint8_t*, size_t,
                          uint32_t*, nghttp2_data_source*, void*);

struct DataBufferSender {
  DataBufferSender(const std::string& data)
      : data{reinterpret_cast<const uint8_t*>(data.data())},
        length{data.length()} {
    nghttp2_provider.read_callback = Nghttp2SendString;
    nghttp2_provider.source.ptr = this;
  }

  nghttp2_data_provider nghttp2_provider{};

  const uint8_t* data{nullptr};
  std::size_t length{0};
  std::size_t pos{0};
};

// implements
// https://nghttp2.org/documentation/types.html#c.nghttp2_data_source_read_callback
ssize_t Nghttp2SendString(nghttp2_session*, int32_t, uint8_t* p, size_t want,
                          uint32_t* flags, nghttp2_data_source* source, void*) {
  auto& data_to_send = *static_cast<DataBufferSender*>(source->ptr);

  const size_t remaining = data_to_send.length - data_to_send.pos;

  // TODO: handle NGHTTP2_DATA_FLAG_NO_COPY
  if (remaining > want) {
    memcpy(p, data_to_send.data + data_to_send.pos, want);
    data_to_send.pos += want;
    return want;
  }
  memcpy(p, data_to_send.data + data_to_send.pos, remaining);
  LOG_ERROR() << "write to nghttp2 = "
              << std::string_view{reinterpret_cast<const char*>(p), remaining};
  *flags = NGHTTP2_DATA_FLAG_NONE;
  *flags |= NGHTTP2_DATA_FLAG_EOF;
  return remaining;
}

nghttp2_nv UnsafeHeaderToNGHeader(std::string_view name, std::string_view value,
                                  const bool sensitive) {
  nghttp2_nv result;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  result.name = reinterpret_cast<uint8_t*>(const_cast<char*>(name.data()));

  result.namelen = name.length();

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast),
  result.value = reinterpret_cast<uint8_t*>(const_cast<char*>(value.data()));
  result.valuelen = value.length();
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

class Http2HeaderWriter {
 public:
  // We must borrow key-value pairs until the `SendResponseToSocket` is called
  explicit Http2HeaderWriter(std::size_t nheaders) : values_(nheaders) {}

  void AddKeyValue(std::string_view key, std::string&& value) {
    values_.push_back(std::move(value));
    ng_headers_.push_back(UnsafeHeaderToNGHeader(key, values_.back(), false));
  }

  void AddKeyValue(std::string_view key, std::string_view value) {
    ng_headers_.push_back(UnsafeHeaderToNGHeader(key, value, false));
  }

  void AddCookie(const Cookie&) {
    // TODO
  }

  std::vector<nghttp2_nv>& Result() { return ng_headers_; }

 private:
  std::vector<std::string> values_;
  std::vector<nghttp2_nv> ng_headers_;
};

class Http2ResponseWriter {
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
    Http2HeaderWriter header_writer{response_.headers_.size() + 3};

    header_writer.AddKeyValue(
        kStatus, fmt::to_string(static_cast<std::uint16_t>(response_.status_)));

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
    for (const auto& item : headers) {
      if (item.first ==
          std::string{USERVER_NAMESPACE::http::headers::kContentLength}) {
        continue;
      }
      header_writer.AddKeyValue(item.first, item.second);
    }
    for (const auto& cookie : response_.cookies_) {
      header_writer.AddCookie(cookie.second);
    }
    return header_writer;
  }

  size_t SendResponseToSocket(nghttp2_session* session,
                              engine::io::RwBase& socket) {
    std::string res;
    while (nghttp2_session_want_write(session)) {
      LOG_ERROR() << "Session want write";
      const std::uint8_t* data_ptr = nullptr;
      const std::size_t data_length =
          nghttp2_session_mem_send(session, &data_ptr);
      LOG_ERROR() << "mem send return data_length=" << data_length;
      const std::string_view a{reinterpret_cast<const char*>(data_ptr),
                               data_length};
      LOG_ERROR() << "[a=]" << a;
      res.append(a);
    }
    LOG_ERROR() << "__ReleaseResponse__" << res;
    return socket.WriteAll(res.data(), res.size(), {});
  }

  void WriteHttp2BodyNotstreamed(engine::io::RwBase& socket,
                                 Http2HeaderWriter& header_writer) {
    LOG_ERROR() << "Send body";
    const bool is_body_forbidden = IsBodyForbiddenForStatus(response_.status_);
    const bool is_head_request =
        response_.request_.GetMethod() == HttpMethod::kHead;
    const auto& data = response_.GetData();

    LOG_ERROR() << "Will send body " << data;
    if (!is_body_forbidden) {
      LOG_ERROR() << "Set body len " << data.size();
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
    const auto& nva = header_writer.Result();
    for (auto h : nva) {
      LOG_ERROR() << "{HEADER}="
                  << std::string_view{reinterpret_cast<char*>(h.name),
                                      h.namelen}
                  << " -> "
                  << std::string_view{reinterpret_cast<char*>(h.value),
                                      h.valuelen};
    }

    const std::uint32_t stream_id = response_.GetStreamId().value();
    if (!is_head_request && !is_body_forbidden) {
      LOG_ERROR() << "Send to stream_id " << stream_id;
      int rv =
          nghttp2_submit_response(session_, stream_id, nva.data(), nva.size(),
                                  &data_sender.nghttp2_provider);
      LOG_ERROR() << "submit_response " << rv;
      UASSERT(rv == 0);
    } else {
      int rv =
          nghttp2_submit_response(session_, response_.GetStreamId().value(),
                                  nva.data(), nva.size(), nullptr);
      LOG_ERROR() << "submit_response " << rv;
      UASSERT(rv == 0);
    }

    const auto sent_bytes = SendResponseToSocket(session_, socket);
    LOG_ERROR() << "bytes= " << sent_bytes << " were send";
    response_.SetSent(sent_bytes, std::chrono::steady_clock::now());
  }

  void WriteHttp2BodyStreamed(engine::io::RwBase&, Http2HeaderWriter&) {
    // TODO
  }

  HttpResponse& response_;
  nghttp2_session* session_;
};

void WriteHttp2ResponseToSocket(engine::io::RwBase& socket,
                                HttpResponse& response,
                                Http2RequestParser& parser) {
  Http2ResponseWriter w{response, parser.GetNghttp2SessionPtr()};
  w.WriteHttpResponse(socket);
}

}  // namespace server::http

USERVER_NAMESPACE_END
