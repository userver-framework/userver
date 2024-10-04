#include <server/http/http2_writer.hpp>

#include <fmt/format.h>
#include <boost/container/small_vector.hpp>

#include <server/http/http2_session.hpp>
#include <server/http/http_cached_date.hpp>

#include <userver/http/common_headers.hpp>
#include <userver/http/predefined_header.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/utils/impl/projecting_view.hpp>
#include <userver/utils/small_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

bool IsBodyForbiddenForStatus(HttpStatus status) {
  return status == HttpStatus::kNoContent ||
         status == HttpStatus::kNotModified ||
         (static_cast<int>(status) >= 100 && static_cast<int>(status) < 200);
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////////

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

class Http2HeaderWriter final {
 public:
  using HeadersVector = boost::container::small_vector<nghttp2_nv, 16>;

  // We must borrow key-value pairs until the `WriteHttpResponse` is called
  explicit Http2HeaderWriter(std::size_t nheaders) {
    values_.reserve(nheaders);
  }

  void AddKeyValue(std::string_view key, std::string&& value) {
    const auto* ptr = values_.data();
    values_.push_back(std::move(value));
    UASSERT(ptr == values_.data());
    ng_headers_.push_back(UnsafeHeaderToNGHeader(key, values_.back(), false));
    bytes_ += (key.size() + value.size());
  }

  void AddKeyValue(std::string_view key, std::string_view value) {
    ng_headers_.push_back(UnsafeHeaderToNGHeader(key, value, false));
    bytes_ += (key.size() + value.size());
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
    const std::string_view key = USERVER_NAMESPACE::http::headers::kSetCookie;
    bytes_ += (key.size() + val.size());
  }

  // A dirty size before HPACK
  std::size_t GetSize() const { return bytes_; }

  HeadersVector& GetNgHeaders() { return ng_headers_; }

 private:
  boost::container::small_vector<std::string, 16> values_;
  HeadersVector ng_headers_;
  std::size_t bytes_{0};
};

class Http2ResponseWriter final {
 public:
  Http2ResponseWriter(HttpResponse& response, Http2Session& session)
      : response_(response), http2_session_(session) {}

  void WriteHttpResponse() {
    auto data = response_.ExtractData();

    auto headers = GetHeaders();
    const bool is_body_forbidden = IsBodyForbiddenForStatus(response_.status_);

    if (is_body_forbidden && !data.empty()) {
      LOG_LIMITED_WARNING()
          << "Non-empty body provided for response with HTTP2 code "
          << static_cast<int>(response_.status_)
          << " which does not allow one, it will be dropped";
    }

    const auto stream_id = response_.GetStreamId().value();
    auto& stream = http2_session_.GetStreamChecked(Stream::Id{stream_id});
    stream.SetStreaming(response_.IsBodyStreamed() && data.empty());

    std::size_t bytes = headers.GetSize();
    nghttp2_data_provider* provider{nullptr};
    if (response_.request_.GetMethod() != HttpMethod::kHead &&
        !is_body_forbidden) {
      if (!stream.IsStreaming()) {
        bytes += data.size();
        stream.PushChunk(std::move(data));
      }
      provider = stream.GetNativeProvider();
    }

    const auto& nva = headers.GetNgHeaders();
    const int rv =
        nghttp2_submit_response(http2_session_.GetNghttp2SessionPtr(),
                                stream_id, nva.data(), nva.size(), provider);
    if (rv != 0) {
      throw std::runtime_error{
          fmt::format("Fail to submit the response with err id = {}. Err: {}",
                      rv, nghttp2_strerror(rv))};
    }

    http2_session_.WriteWhileWant();
    response_.SetSent(bytes, std::chrono::steady_clock::now());
  }

 private:
  Http2HeaderWriter GetHeaders() const {
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
                                kDefaultContentType);
    }
    std::string_view content_lenght_header{
        USERVER_NAMESPACE::http::headers::kContentLength};
    for (const auto& [key, value] : headers) {
      if (key == content_lenght_header) {
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

 private:
  HttpResponse& response_;
  Http2Session& http2_session_;
};

void WriteHttp2ResponseToSocket(HttpResponse& response, Http2Session& session) {
  Http2ResponseWriter w{response, session};
  w.WriteHttpResponse();
}

}  // namespace server::http

USERVER_NAMESPACE_END
