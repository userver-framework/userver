#include "http_response_writer.hpp"

#include <fmt/compile.h>
#include <nghttp2/nghttp2.h>

#include <server/http/http_cached_date.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_response.hpp>

#include "http_request_impl.hpp"
#include "userver/logging/log.hpp"

#include <iostream>
#include <list>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kCrlf = "\r\n";
constexpr std::string_view kKeyValueHeaderSeparator = ": ";

const auto kDefaultContentTypeString =
    http::ContentType{"text/html; charset=utf-8"}.ToString();

constexpr std::string_view kClose = "close";
constexpr std::string_view kKeepAlive = "keep-alive";
constexpr std::string_view kUpgradeResponse =
    "HTTP/{}.{} 101 Switching Protocols\r\n"
    "Connection: Upgrade\r\n"
    "Upgrade: h2c\r\n\r\n";

bool IsBodyForbiddenForStatus(server::http::HttpStatus status) {
  return status == server::http::HttpStatus::kNoContent ||
         status == server::http::HttpStatus::kNotModified ||
         (static_cast<int>(status) >= 100 && static_cast<int>(status) < 200);
}

void OutputHeader(std::string& header, std::string_view key,
                  std::string_view val) {
  const auto old_size = header.size();
  header.resize(old_size + key.size() + kKeyValueHeaderSeparator.size() +
                val.size() + kCrlf.size());

  char* append_position = header.data() + old_size;
  const auto append = [&append_position](std::string_view what) {
    std::memcpy(append_position, what.data(), what.size());
    append_position += what.size();
  };

  append(key);
  append(kKeyValueHeaderSeparator);
  append(val);
  append(kCrlf);
}

struct DataFrame {
  std::array<uint8_t, 10> frame_header{};
  size_t frame_header_size = 0;
  const uint8_t* data{};
  size_t data_length = 0;
  std::vector<uint8_t> padding{};
};

ssize_t Nghttp2SendString(nghttp2_session*, int32_t, uint8_t*, size_t,
                          uint32_t*, nghttp2_data_source*, void*);

struct DataBufferSender {
  DataBufferSender(const std::string& data)
      : nghttp2_provider{{.ptr = this}, Nghttp2SendString},
        data{reinterpret_cast<const uint8_t*>(data.data())},
        length{data.length()} {}

  nghttp2_data_provider nghttp2_provider{};

  const uint8_t* data{nullptr};
  size_t length{0};
  size_t pos{0};

  DataFrame data_frame_to_send{};
  bool has_data_frame_to_send{false};
};

// implements
// https://nghttp2.org/documentation/types.html#c.nghttp2_data_source_read_callback
ssize_t Nghttp2SendString(nghttp2_session*, int32_t, uint8_t*, size_t want,
                          uint32_t* flags, nghttp2_data_source* source, void*) {
  const auto& data_to_send = *static_cast<DataBufferSender*>(source->ptr);

  const size_t remaining = data_to_send.length - data_to_send.pos;
  if (remaining > 0) {
    *flags |= NGHTTP2_DATA_FLAG_NO_COPY;
  }
  if (remaining > want) {
    return want;
  }
  *flags |= NGHTTP2_DATA_FLAG_EOF;
  return remaining;
}

nghttp2_nv UnsafeHeaderToNGHeader(std::string_view name, std::string_view value,
                                  const bool sensitive) {
  nghttp2_nv result;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  result.name = reinterpret_cast<uint8_t*>(const_cast<char*>(name.data()));

  // ??? length - 1
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

namespace server::http {

namespace nghttp2_callbacks::send {

int OnSendDataCallback(nghttp2_session*, nghttp2_frame* frame,
                       const uint8_t* framehd, size_t length,
                       nghttp2_data_source* source, void*) {
  std::cout << "OnSendDataCallback called for stream " << frame->hd.stream_id;
  LOG_DEBUG() << "OnSendDataCallback called for stream " << frame->hd.stream_id;
  UASSERT(false);
  auto& data_to_send = *static_cast<DataBufferSender*>(source->ptr);

  // see
  // https://nghttp2.org/documentation/types.html#c.nghttp2_send_data_callback
  auto& res = data_to_send.data_frame_to_send;

  std::memcpy(res.frame_header.data(), framehd, 9);
  if (frame->data.padlen > 0) {
    res.frame_header[9] = static_cast<uint8_t>(frame->data.padlen - 1);
    res.frame_header_size = 10;
  } else {
    res.frame_header_size = 9;
  }

  res.data = data_to_send.data + data_to_send.pos;
  res.data_length = length;
  data_to_send.pos += length;

  if (frame->data.padlen > 1) {
    res.padding.assign(frame->data.padlen - 1, 0);
  } else {
    res.padding.resize(0);
  }

  data_to_send.has_data_frame_to_send = true;

  return 0;
}

}  // namespace nghttp2_callbacks::send

class Http1HeaderWriter {
 public:
  Http1HeaderWriter() {
    // According to https://www.chromium.org/spdy/spdy-whitepaper/
    // "typical header sizes of 700-800 bytes is common"
    // Adjusting it to 1KiB to fit jemalloc size class
    static constexpr auto kTypicalHeadersSize = 1024;
    header_.reserve(kTypicalHeadersSize);
  }

  void WriteVersionAndStatus(const HttpRequestImpl& request,
                             const HttpStatus status) {
    header_.append("HTTP/");
    fmt::format_to(std::back_inserter(header_), FMT_COMPILE("{}.{} {} "),
                   request.GetHttpMajor(), request.GetHttpMinor(),
                   static_cast<int>(status));
    header_.append(HttpStatusString(status));
    header_.append(kCrlf);
  }

  void AddKeyValue(const std::string_view key, const std::string_view value) {
    OutputHeader(header_, key, value);
  }

  void AddCookie(const Cookie& cookie) {
    header_.append(USERVER_NAMESPACE::http::headers::kSetCookie);
    header_.append(kKeyValueHeaderSeparator);
    cookie.AppendToString(header_);
    header_.append(kCrlf);
  }

  void NotStreamedFinalize() { header_.append(kCrlf); }

  std::string& Result() { return header_; }

 private:
  std::string header_{};
};

class Http2HeaderWriter {
 public:
  // IMPORTANT: we assume, that key will not be deleted before
  // Http2HeaderWriter, so we can store reference to the key.
  void AddKeyValue(const std::string_view key, std::string&& value) {
    value_storage_.push_back(std::move(value));
    ng_headers_.push_back(
        UnsafeHeaderToNGHeader(key, value_storage_.back(), false));
  }

  void AddKeyValue(const std::string_view key, const std::string_view value) {
    value_storage_.emplace_back(value);
    ng_headers_.push_back(
        UnsafeHeaderToNGHeader(key, value_storage_.back(), false));
  }

  void AddCookie(const Cookie& cookie) {
    value_storage_.emplace_back();
    cookie.AppendToString(value_storage_.back());
    ng_headers_.push_back(
        UnsafeHeaderToNGHeader(USERVER_NAMESPACE::http::headers::kSetCookie,
                               value_storage_.back(), true));
  }

  std::vector<nghttp2_nv>& Result() { return ng_headers_; }

 private:
  std::list<std::string> value_storage_{};

  std::vector<nghttp2_nv> ng_headers_;
};

template <typename HttpHeaderWriter>
class HttpResponseWriter {
 public:
  HttpResponseWriter(server::http::HttpResponse& response)
      : response_(response) {}

 protected:
  void WriteCommonHeaders(HttpHeaderWriter& header_writer) {
    const auto& headers = response_.headers_;

    const auto end = headers.cend();
    if (headers.find(USERVER_NAMESPACE::http::headers::kDate) == end) {
      header_writer.AddKeyValue(USERVER_NAMESPACE::http::headers::kDate,
                                GetCachedDate());
    }
    if (headers.find(USERVER_NAMESPACE::http::headers::kContentType) == end) {
      header_writer.AddKeyValue(USERVER_NAMESPACE::http::headers::kContentType,
                                kDefaultContentTypeString);
    }
    for (const auto& item : headers) {
      if (item.first == USERVER_NAMESPACE::http::headers::kContentLength) {
        continue;
      }

      header_writer.AddKeyValue(item.first, item.second);
    }
  }

  void WriteCookies(HttpHeaderWriter& header_writer) {
    for (const auto& cookie : response_.cookies_) {
      header_writer.AddCookie(cookie.second);
    }
  }

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  server::http::HttpResponse& response_;
};

class Http1ResponseWriter : HttpResponseWriter<Http1HeaderWriter> {
 public:
  using HttpResponseWriter<Http1HeaderWriter>::HttpResponseWriter;

  void WriteHttpResponse(engine::io::Socket& socket) {
    Http1HeaderWriter header_writer;
    header_writer.WriteVersionAndStatus(response_.request_, response_.status_);
    WriteCommonHeaders(header_writer);
    WriteHttp1SpecificHeaders(header_writer);
    WriteCookies(header_writer);

    if (response_.IsBodyStreamed() && response_.GetData().empty()) {
      WriteHttp1BodyStreamed(socket, header_writer.Result());
    } else {
      // e.g. a CustomHandlerException
      WriteHttp1BodyNotstreamed(socket, header_writer.Result());
    }
  }

 private:
  void WriteHttp1SpecificHeaders(Http1HeaderWriter& header_writer) {
    const auto& headers = response_.headers_;
    if (headers.find(USERVER_NAMESPACE::http::headers::kConnection) ==
        headers.end()) {
      header_writer.AddKeyValue(
          USERVER_NAMESPACE::http::headers::kConnection,
          (response_.request_.IsFinal() ? kClose : kKeepAlive));
    }
  }

  void WriteHttp1BodyNotstreamed(engine::io::Socket& socket,
                                 std::string& header) {
    const bool is_body_forbidden = IsBodyForbiddenForStatus(response_.status_);
    const bool is_head_request =
        response_.request_.GetOrigMethod() == HttpMethod::kHead;
    const auto& data = response_.GetData();

    if (!is_body_forbidden) {
      OutputHeader(header, USERVER_NAMESPACE::http::headers::kContentLength,
                   fmt::format(FMT_COMPILE("{}"), data.size()));
    }
    header.append(kCrlf);

    if (is_body_forbidden && !data.empty()) {
      LOG_LIMITED_WARNING()
          << "Non-empty body provided for response with HTTP code "
          << static_cast<int>(response_.status_)
          << " which does not allow one, it will be dropped";
    }

    ssize_t sent_bytes = 0;
    if (!is_head_request && !is_body_forbidden) {
      sent_bytes = socket.SendAll(
          {{header.data(), header.size()}, {data.data(), data.size()}},
          engine::Deadline{});
    } else {
      sent_bytes =
          socket.SendAll(header.data(), header.size(), engine::Deadline{});
    }

    response_.SetSentTime(std::chrono::steady_clock::now());
    response_.SetSent(sent_bytes);
  }

  void WriteHttp1BodyStreamed(engine::io::Socket& socket, std::string& header) {
    OutputHeader(header, USERVER_NAMESPACE::http::headers::kTransferEncoding,
                 "chunked");

    // send HTTP headers
    size_t sent_bytes = socket.SendAll(header.data(), header.size(), {});
    std::string().swap(header);  // free memory before time consuming operation

    // Transmit HTTP response body
    std::string body_part;
    while (response_.body_stream_->Pop(body_part)) {
      if (body_part.empty()) {
        LOG_DEBUG() << "Zero size body_part in http_response.cpp";
        continue;
      }

      auto size = fmt::format("\r\n{:x}\r\n", body_part.size());
      sent_bytes += socket.SendAll(
          {{size.data(), size.size()}, {body_part.data(), body_part.size()}},
          engine::Deadline{});
    }

    const constexpr std::string_view terminating_chunk{"\r\n0\r\n\r\n"};
    sent_bytes +=
        socket.SendAll(terminating_chunk.data(), terminating_chunk.size(), {});

    // TODO: exceptions?
    response_.body_stream_producer_.reset();
    response_.body_stream_.reset();

    response_.SetSentTime(std::chrono::steady_clock::now());
    response_.SetSent(sent_bytes);
  }
};

class Http2ResponseWriter : HttpResponseWriter<Http2HeaderWriter> {
 public:
  // using HttpResponseWriter<Http2HeaderWriter>::HttpResponseWriter;

  Http2ResponseWriter(HttpResponse& response,
                      concurrent::Variable<impl::SessionPtr>& session_holder,
                      std::optional<Http2UpgradeData> upgrade_data)
      : HttpResponseWriter<Http2HeaderWriter>{response},
        session_holder_{session_holder},
        upgrade_data_{std::move(upgrade_data)},
        need_send_settings_{upgrade_data_.has_value()},
        need_send_switching_protocol_{upgrade_data_.has_value()} {}

  void WriteHttpResponse(engine::io::Socket& socket) {
    Http2HeaderWriter header_writer;
    header_writer.AddKeyValue(
        USERVER_NAMESPACE::http2::pseudo_headers::kStatus,
        fmt::format(FMT_COMPILE("{}"), static_cast<int>(response_.status_)));
    WriteCommonHeaders(header_writer);
    WriteCookies(header_writer);

    if (response_.IsBodyStreamed() && response_.GetData().empty()) {
      // to do
      WriteHttp2BodyStreamed(socket, header_writer);
    } else {
      // e.g. a CustomHandlerException
      WriteHttp2BodyNotstreamed(socket, header_writer);
    }
  }

 private:
  size_t WriteNGHttp2(nghttp2_session* session,
                      std::optional<DataBufferSender>& data_sender,
                      engine::io::Socket& socket) {
    std::vector<engine::io::IoData> io_socket;
    const auto add_view = [&](const void* data, const size_t length) {
      io_socket.push_back({data, length});
    };
    std::list<std::vector<uint8_t>> io_data_storage;
    const auto add_copy = [&](const uint8_t* data, const size_t length) {
      auto& vec = io_data_storage.emplace_back();
      vec.assign(data, data + length);
      add_view(vec.data(), vec.size());
    };

    WriteSwitchingProtocolIfNeeded(add_copy);

    while (nghttp2_session_want_write(session)) {
      LOG_DEBUG() << "Session want write";
      const uint8_t* data_ptr = nullptr;
      size_t data_length = nghttp2_session_mem_send(session, &data_ptr);
      LOG_DEBUG() << "mem send return " << data_length;
      if (data_sender && data_sender->has_data_frame_to_send) {
        const auto& frame = data_sender->data_frame_to_send;
        add_copy(frame.frame_header.data(), frame.frame_header_size);
        add_view(frame.data, frame.data_length);
        if (!frame.padding.empty()) {
          add_copy(frame.padding.data(), frame.padding.size());
        }
        data_sender->has_data_frame_to_send = false;
      } else if (data_length > 0) {
        add_copy(data_ptr, data_length);
      }
    }

    return socket.SendAll(io_socket.data(), io_socket.size(), {});
  }

  void WriteHttp2BodyNotstreamed(engine::io::Socket& socket,
                                 Http2HeaderWriter& header_writer) {
    LOG_DEBUG() << "Send body";
    const bool is_body_forbidden = IsBodyForbiddenForStatus(response_.status_);
    const bool is_head_request =
        response_.request_.GetOrigMethod() == HttpMethod::kHead;
    const auto& data = response_.GetData();

    LOG_DEBUG() << "Will send body " << data;
    if (!is_body_forbidden) {
      LOG_DEBUG() << "Set body len " << data.size();
      header_writer.AddKeyValue(
          USERVER_NAMESPACE::http::headers::kContentLength,
          fmt::format(FMT_COMPILE("{}"), data.size()));
    }

    if (is_body_forbidden && !data.empty()) {
      LOG_LIMITED_WARNING()
          << "Non-empty body provided for response with HTTP code "
          << static_cast<int>(response_.status_)
          << " which does not allow one, it will be dropped";
    }

    std::optional<DataBufferSender> data_sender;
    const auto& nva = header_writer.Result();

    const auto session = session_holder_.Lock();
    SubmitServerSettingsIfNeeded(session->get());

    uint32_t stream_id = response_.stream_id;
    if (!is_head_request && !is_body_forbidden) {
      data_sender.emplace(data);
      LOG_DEBUG() << "Send to stream_id " << stream_id;
      int rv =
          nghttp2_submit_response(session->get(), stream_id, nva.data(),
                                  nva.size(), &data_sender->nghttp2_provider);
      LOG_DEBUG() << "submit_response " << rv;
      UASSERT(!rv);
    } else {
      int rv = nghttp2_submit_response(session->get(), response_.stream_id,
                                       nva.data(), nva.size(), nullptr);
      UASSERT(!rv);
    }

    const auto sent_bytes = WriteNGHttp2(session->get(), data_sender, socket);
    LOG_DEBUG() << sent_bytes << " was send";
    response_.SetSentTime(std::chrono::steady_clock::now());
    response_.SetSent(sent_bytes);
  }

  void WriteHttp2BodyStreamed(engine::io::Socket& socket,
                              Http2HeaderWriter& header_writer) {
    size_t sent_bytes = 0;
    uint32_t stream_id = response_.stream_id;
    std::optional<DataBufferSender> data_sender;
    bool headers_sent = false;

    const auto& nva = header_writer.Result();
    const auto submit_headers_if_needed = [&](nghttp2_session* session,
                                              const bool end_of_stream) {
      if (headers_sent) {
        return;
      }
      headers_sent = true;
      int rv = nghttp2_submit_headers(
          session, end_of_stream ? NGHTTP2_FLAG_END_STREAM : NGHTTP2_FLAG_NONE,
          stream_id, nullptr, nva.data(), nva.size(), nullptr);
      UASSERT(rv == 0);
    };
    const auto submit_data = [&](nghttp2_session* session,
                                 const std::string& data,
                                 const bool end_of_stream) {
      data_sender.emplace(data);
      int rv = nghttp2_submit_data(
          session, end_of_stream ? NGHTTP2_FLAG_END_STREAM : NGHTTP2_FLAG_NONE,
          stream_id, &data_sender->nghttp2_provider);
      UASSERT(rv == 0);
    };

    const auto get_next_body_part = [&](std::string& body_part) {
      while (response_.body_stream_->Pop(body_part)) {
        if (body_part.empty()) {
          LOG_DEBUG() << "Zero size body_part in http_response.cpp";
          continue;
        }
        return true;
      }
      return false;
    };

    const auto submit_all = [&](const std::string& data,
                                const bool end_of_stream) {
      const auto session = session_holder_.Lock();
      SubmitServerSettingsIfNeeded(session->get());
      if (data.empty()) {
        submit_headers_if_needed(session->get(), end_of_stream);
      } else {
        submit_headers_if_needed(session->get(), false);
        submit_data(session->get(), data, end_of_stream);
      }
      // todo: write_to_socket is very bad. replace with normal sending
      sent_bytes += WriteNGHttp2(session->get(), data_sender, socket);
    };

    std::string body_part;
    std::string next_body_part;
    if (get_next_body_part(body_part)) {
      while (get_next_body_part(next_body_part)) {
        submit_all(body_part, false);
        body_part.resize(0);
        std::swap(body_part, next_body_part);
      }
    }
    UASSERT(next_body_part.empty());
    submit_all(body_part, true);

    // TODO: exceptions?
    response_.body_stream_producer_.reset();
    response_.body_stream_.reset();

    response_.SetSentTime(std::chrono::steady_clock::now());
    response_.SetSent(sent_bytes);
  }

  void SubmitServerSettingsIfNeeded([[maybe_unused]] nghttp2_session* session) {
    if (!need_send_settings_) {
      return;
    }
    need_send_settings_ = false;

    const auto& server_settings = upgrade_data_->server_settings.get();
    int rv =
    nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE,
    server_settings.data(), server_settings.size());
    UASSERT_MSG(rv == 0, "Error during sending http2 settings");
  }

  template <typename WriteFunc>
  void WriteSwitchingProtocolIfNeeded(WriteFunc write_func) {
    if (!need_send_switching_protocol_) {
      return;
    }
    need_send_switching_protocol_ = false;

    std::string upgrade_response =
        fmt::format(kUpgradeResponse, response_.request_.GetHttpMajor(),
                    response_.request_.GetHttpMinor());
    LOG_DEBUG() << "Try to send upgrade";
    write_func(reinterpret_cast<uint8_t*>(upgrade_response.data()),
               upgrade_response.size());
    LOG_DEBUG() << "Upgrade sended";
  }

  concurrent::Variable<impl::SessionPtr>& session_holder_;
  std::optional<Http2UpgradeData> upgrade_data_{};

  bool need_send_settings_{false};
  bool need_send_switching_protocol_{false};
};

void WriteHttp1Response(engine::io::Socket& socket,
                        server::http::HttpResponse& response) {
  Http1ResponseWriter response_writer{response};
  response_writer.WriteHttpResponse(socket);
}

void WriteHttp2Response(engine::io::Socket& socket,
                        server::http::HttpResponse& response,
                        concurrent::Variable<impl::SessionPtr>& session_holder,
                        std::optional<Http2UpgradeData> upgrade_data) {
  Http2ResponseWriter response_writer{response, session_holder,
                                      std::move(upgrade_data)};
  response_writer.WriteHttpResponse(socket);
}

}  // namespace server::http

USERVER_NAMESPACE_END
