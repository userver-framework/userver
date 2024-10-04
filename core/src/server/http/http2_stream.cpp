#include <server/http/http2_stream.hpp>

#include <userver/engine/io/socket.hpp>

#include <numeric>  // std::accumulate

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

// implements
// https://nghttp2.org/documentation/types.html#c.nghttp2_data_source_read_callback
ssize_t NgHttp2ReadCallback(nghttp2_session*, int32_t, uint8_t*,
                            std::size_t max_len, uint32_t* flags,
                            nghttp2_data_source* source, void*) {
  UASSERT(flags);
  UASSERT(source);
  UASSERT(source->ptr);
  auto& stream = *static_cast<Stream*>(source->ptr);
  *flags = NGHTTP2_DATA_FLAG_NONE;
  *flags |= NGHTTP2_DATA_FLAG_NO_COPY;
  return stream.GetMaxSize(max_len, flags);
}

}  // namespace

Stream::Stream(HttpRequestConstructor::Config config,
               const HandlerInfoIndex& handler_info_index,
               request::ResponseDataAccounter& data_accounter,
               engine::io::Sockaddr remote_address, Id id)
    : constructor_(config, handler_info_index, data_accounter, remote_address),
      id_(id) {
  constructor_.SetHttpMajor(2);
  constructor_.SetHttpMinor(0);
  nghttp2_provider_.read_callback = NgHttp2ReadCallback;
  nghttp2_provider_.source.ptr = this;
}

Stream::Id Stream::GetId() const { return id_; }

HttpRequestConstructor& Stream::RequestConstructor() { return constructor_; }

bool Stream::IsDeferred() const { return is_deferred_; }

void Stream::SetDeferred(bool deferred) { is_deferred_ = deferred; }

void Stream::SetEnd(bool end) { is_end_ = end; }

bool Stream::IsStreaming() const { return is_streaming_; }

void Stream::SetStreaming(bool streaming) { is_streaming_ = streaming; }

bool Stream::CheckUrlComplete() {
  if (url_complete_) return true;
  try {
    constructor_.ParseUrl();
  } catch (const std::exception& e) {
    LOG_LIMITED_WARNING() << "Can't parse a url: " << e;
    return false;
  }
  url_complete_ = true;
  return true;
}

void Stream::PushChunk(std::string&& chunk) {
  if (chunk.empty()) return;
  chunks_.push_back(std::move(chunk));
}

ssize_t Stream::GetMaxSize(std::size_t max_len, std::uint32_t* flags) {
  auto& stream = *static_cast<Stream*>(nghttp2_provider_.source.ptr);
  if (chunks_.empty() && !stream.is_streaming_) {
    *flags |= NGHTTP2_DATA_FLAG_EOF;
    return 0;
  }
  const std::size_t total = std::accumulate(
      chunks_.begin(), chunks_.end(), std::size_t{0},
      [](std::size_t size, const auto& str) { return size + str.size(); });
  if (total == 0 && stream.is_streaming_ && !stream.is_end_) {
    stream.is_deferred_ = true;
    return NGHTTP2_ERR_DEFERRED;
  }
  if (!stream.is_streaming_) {
    UASSERT(chunks_.size() == 1);
    if (pos_in_first_chunk_ + max_len >= chunks_[0].size()) {
      *flags |= NGHTTP2_DATA_FLAG_EOF;
    }
    return std::min(max_len, chunks_[0].size() - pos_in_first_chunk_);
  }
  UASSERT(total >= pos_in_first_chunk_);
  const auto remaining = total - pos_in_first_chunk_;
  const auto res = std::min(remaining, max_len);
  if (stream.is_end_ && res >= remaining) {
    *flags |= NGHTTP2_DATA_FLAG_EOF;
  }
  return res;
}

void Stream::Send(engine::io::Socket& socket,
                  std::string_view data_frame_header, std::size_t max_len) {
  boost::container::small_vector<engine::io::IoData, 16> parts{};
  parts.push_back({data_frame_header.data(), data_frame_header.size()});
  auto budget = max_len;
  for (const auto& chunk : chunks_) {
    if (budget == 0) {
      break;
    }
    UASSERT(chunk.size() > pos_in_first_chunk_);
    const auto part = std::string_view{chunk}.substr(
        pos_in_first_chunk_,
        std::min(chunk.size() - pos_in_first_chunk_, budget));
    parts.push_back({part.data(), part.size()});
    pos_in_first_chunk_ += part.size();
    if (pos_in_first_chunk_ >= chunk.size()) {
      pos_in_first_chunk_ = 0;
    }
    UASSERT(budget >= part.size());
    budget -= part.size();
  }
  UASSERT(!parts.empty());
  [[maybe_unused]] const auto res =
      socket.SendAll(parts.data(), parts.size(), {});
  const auto send_parts_count =
      pos_in_first_chunk_ == 0
          ? parts.size() - 1
          : parts.size() - 2;  // data_frame_header doesn't matter
  chunks_.erase(chunks_.begin(), chunks_.begin() + send_parts_count);
}

}  // namespace server::http

USERVER_NAMESPACE_END
