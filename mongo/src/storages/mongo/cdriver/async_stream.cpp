#include <storages/mongo/cdriver/async_stream.hpp>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

#include <userver/clients/dns/exception.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/local_variable.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/assert.hpp>

#include <storages/mongo/cdriver/async_stream_poller.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/tcp_connect_precheck.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {
namespace {

// chosen empirically as the best performance for size (16K-32K)
constexpr size_t kBufferSize = 32 * 1024;

constexpr int kCompatibleMajorVersion = 1;
constexpr int kMaxCompatibleMinorVersion = 21;  // Tested on Fedora, works

using FunctionPtr = void (*)(mongoc_stream_t* stream);

struct ExpectedMongocStreamLayout {
  int type;
  FunctionPtr destroy, close, flush, writev, readv, setsockopt, get_base_stream,
      check_closed, poll, failed, timed_out, should_retry;
  void* padding[3];
};

static_assert(sizeof(ExpectedMongocStreamLayout) == sizeof(mongoc_stream_t),
              "Unexpected mongoc_stream_t structure layout");

static_assert(std::size(mongoc_stream_t{}.padding) == 3,
              "Unexpected mongoc_stream_t structure layout");

void SetWatcher(AsyncStreamPoller::WatcherPtr& old_watcher,
                AsyncStreamPoller::WatcherPtr new_watcher) {
  if (old_watcher == new_watcher) return;
  if (old_watcher) old_watcher->Stop();
  old_watcher = std::move(new_watcher);
}

class AsyncStream : public mongoc_stream_t {
 public:
  static constexpr int kStreamType = 0x53755459;

  static cdriver::StreamPtr Create(engine::io::Socket);

  void SetCreated() { is_created_ = true; }

 private:
  AsyncStream(engine::io::Socket) noexcept;

  // mongoc_stream_buffered resizes itself indiscriminately
  // NOTE: returns number of bytes stored to data, not buffered!
  size_t BufferedRecv(void* data, size_t size, size_t min_bytes,
                      engine::Deadline deadline);

  // mongoc_stream_t interface
  static void Destroy(mongoc_stream_t*) noexcept;
  static int Close(mongoc_stream_t*) noexcept;
  static int Flush(mongoc_stream_t*) noexcept;
  static ssize_t Writev(mongoc_stream_t*, mongoc_iovec_t*, size_t,
                        int32_t) noexcept;
  static ssize_t Readv(mongoc_stream_t*, mongoc_iovec_t*, size_t, size_t,
                       int32_t) noexcept;
  static int Setsockopt(mongoc_stream_t*, int, int, void*,
                        mongoc_socklen_t) noexcept;
  static bool CheckClosed(mongoc_stream_t*) noexcept;
  static ssize_t Poll(mongoc_stream_poll_t*, size_t, int32_t) noexcept;
  static void Failed(mongoc_stream_t*) noexcept;
  static bool TimedOut(mongoc_stream_t*) noexcept;
  static bool ShouldRetry(mongoc_stream_t*) noexcept;

  const uint64_t epoch_;
  engine::io::Socket socket_;
  AsyncStreamPoller::WatcherPtr read_watcher_;
  AsyncStreamPoller::WatcherPtr write_watcher_;
  bool is_timed_out_{false};
  bool is_created_{false};

  size_t recv_buffer_bytes_used_{0};
  size_t recv_buffer_pos_{0};

  // buffer sizes are adjusted for better heap utilization and aligned for copy
  static constexpr size_t kAlignment = 256;
  static_assert(kBufferSize % kAlignment == 0);
  alignas(kAlignment) std::array<char, kBufferSize - kAlignment> recv_buffer_;
};

engine::Deadline DeadlineFromTimeoutMs(int32_t timeout_ms) {
  if (timeout_ms < 0) return {};
  if (!timeout_ms) {
    return engine::Deadline::Passed();
  }
  return engine::Deadline::FromDuration(std::chrono::milliseconds(timeout_ms));
}

engine::io::Socket ConnectUnix(const mongoc_host_list_t& host,
                               int32_t timeout_ms, bson_error_t* error) {
  engine::io::Sockaddr addr;
  auto* sa = addr.As<sockaddr_un>();
  sa->sun_family = AF_UNIX;

  const auto host_len = std::strlen(host.host);
  if (host_len >= sizeof(sa->sun_path)) {
    bson_set_error(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_CONNECT,
                   "Cannot connect to UNIX socket '%s': path too long",
                   host.host);
    return {};
  }
  std::memcpy(sa->sun_path, host.host, host_len);

  try {
    engine::TaskCancellationBlocker block_cancel;
    engine::io::Socket socket{addr.Domain(), engine::io::SocketType::kStream};
    socket.Connect(addr, DeadlineFromTimeoutMs(timeout_ms));
    return socket;
  } catch (const engine::io::IoCancelled& ex) {
    UASSERT_MSG(false,
                "Cancellation is not supported in cdriver implementation");

    bson_set_error(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_CONNECT,
                   "%s", ex.what());
    return {};
  } catch (const std::exception& ex) {
    LOG_INFO() << "Cannot connect to UNIX socket '" << host.host << "': " << ex;
  }
  bson_set_error(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_CONNECT,
                 "Cannot connect to UNIX socket '%s'", host.host);
  return {};
}

struct AddrinfoDeleter {
  void operator()(struct addrinfo* p) const noexcept { freeaddrinfo(p); }
};
using AddrinfoPtr = std::unique_ptr<struct addrinfo, AddrinfoDeleter>;

clients::dns::AddrVector GetaddrInfo(const mongoc_host_list_t& host,
                                     bson_error_t*) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  struct addrinfo hints;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = host.family;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo* ai_result_raw = nullptr;
  LOG_DEBUG() << "Trying to resolve " << host.host_and_port;
  const auto port_string = std::to_string(host.port);
  if (getaddrinfo(host.host, port_string.c_str(), &hints, &ai_result_raw)) {
    ReportTcpConnectError(host.host_and_port);
    return {};
  }

  clients::dns::AddrVector result;
  AddrinfoPtr ai_result(ai_result_raw);
  for (auto* res = ai_result.get(); res; res = res->ai_next) {
    engine::io::Sockaddr current_addr(res->ai_addr);
    result.push_back(current_addr);
  }
  return result;
}

engine::io::Socket ConnectTcpByName(const mongoc_host_list_t& host,
                                    int32_t timeout_ms, bson_error_t* error,
                                    clients::dns::Resolver* dns_resolver) {
  if (!IsTcpConnectAllowed(host.host_and_port)) {
    bson_set_error(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_CONNECT,
                   "Too many connection errors in recent period for %s",
                   host.host_and_port);
    return {};
  }

  const auto deadline = DeadlineFromTimeoutMs(timeout_ms);
  try {
    auto addrs = dns_resolver ? dns_resolver->Resolve(host.host, deadline)
                              : GetaddrInfo(host, error);
    for (auto&& current_addr : addrs) {
      try {
        engine::TaskCancellationBlocker block_cancel;
        current_addr.SetPort(host.port);
        engine::io::Socket socket{current_addr.Domain(),
                                  engine::io::SocketType::kStream};
        socket.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
        socket.Connect(current_addr, deadline);
        ReportTcpConnectSuccess(host.host_and_port);
        return socket;
      } catch (const engine::io::IoCancelled& ex) {
        UASSERT_MSG(false,
                    "Cancellation is not supported in cdriver implementation");

        bson_set_error(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_CONNECT,
                       "%s", ex.what());
        return {};
      } catch (const engine::io::IoException& ex) {
        LOG_DEBUG() << "Cannot connect to " << host.host << " at "
                    << current_addr << ": " << ex;
      }
    }
  } catch (const clients::dns::ResolverException& ex) {
    LOG_LIMITED_ERROR() << "Cannot resolve " << host.host << ": " << ex;
    bson_set_error(error, MONGOC_ERROR_STREAM,
                   MONGOC_ERROR_STREAM_NAME_RESOLUTION, "Cannot resolve %s",
                   host.host_and_port);
    return {};
  } catch (const std::exception& ex) {
    LOG_LIMITED_ERROR() << "Cannot connect to " << host.host << ": " << ex;
  }
  ReportTcpConnectError(host.host_and_port);
  bson_set_error(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_CONNECT,
                 "Cannot connect to %s", host.host_and_port);
  return {};
}

engine::io::Socket Connect(const mongoc_host_list_t* host, int32_t timeout_ms,
                           bson_error_t* error,
                           clients::dns::Resolver* dns_resolver) {
  UASSERT(host);
  switch (host->family) {
    case AF_UNSPEC:  // mongoc thinks this is okay
    case AF_INET:
    case AF_INET6:
      return ConnectTcpByName(*host, timeout_ms, error, dns_resolver);

    case AF_UNIX:
      return ConnectUnix(*host, timeout_ms, error);

    default:
      bson_set_error(error, MONGOC_ERROR_STREAM,
                     MONGOC_ERROR_STREAM_INVALID_TYPE,
                     "Invalid address family 0x%02x", host->family);
  }
  return {};
}

uint64_t GetNextStreamEpoch() {
  static std::atomic<uint64_t> current_epoch{0};
  return current_epoch++;
}

// We need to reset the poller because of fd reuse and to wipe stale events.
// This operation syncs on ev loops so we want it to be done
// as rarely as possible.
//
// mongoc uses poll only on freshly made streams, we use that knowledge
// to only reset poller when a new poll cycle begins.
class PollerDispenser {
 public:
  AsyncStreamPoller& Get(uint64_t current_epoch) {
    if (seen_epoch_ < current_epoch) {
      poller_.Reset();
      seen_epoch_ = current_epoch;
    }
    return poller_;
  }

 private:
  uint64_t seen_epoch_{0};
  AsyncStreamPoller poller_;
};

engine::TaskLocalVariable<PollerDispenser> poller_dispenser;

}  // namespace

void CheckAsyncStreamCompatible() {
  if (mongoc_get_major_version() != kCompatibleMajorVersion ||
      mongoc_get_minor_version() > kMaxCompatibleMinorVersion) {
    throw std::runtime_error(
        "This implementation of AsyncStream was not checked with "
        "libmongoc " MONGOC_VERSION_S
        " and may be binary incompatible with it, please downgrade to "
        "version " +
        std::to_string(kCompatibleMajorVersion) + '.' +
        std::to_string(kMaxCompatibleMinorVersion));
  }
}

mongoc_stream_t* MakeAsyncStream(const mongoc_uri_t* uri,
                                 const mongoc_host_list_t* host,
                                 void* user_data,
                                 bson_error_t* error) noexcept {
  auto* init_data = static_cast<AsyncStreamInitiatorData*>(user_data);

  const auto connect_timeout_ms =
      mongoc_uri_get_option_as_int32(uri, MONGOC_URI_CONNECTTIMEOUTMS, 5000);
  auto socket =
      Connect(host, connect_timeout_ms, error, init_data->dns_resolver);
  if (!socket) return nullptr;

  auto stream = AsyncStream::Create(std::move(socket));

  // from mongoc_client_default_stream_initiator
  // enable TLS if needed
  const char* mechanism = mongoc_uri_get_auth_mechanism(uri);
  if (mongoc_uri_get_tls(uri) ||
      (mechanism && !std::strcmp(mechanism, "MONGODB-X509"))) {
    {
      cdriver::StreamPtr wrapped_stream(mongoc_stream_tls_new_with_hostname(
          stream.get(), host->host, &init_data->ssl_opt, true));
      if (!wrapped_stream) {
        bson_set_error(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET,
                       "Cannot initialize TLS stream");
        return nullptr;
      }
      [[maybe_unused]] auto* ptr = stream.release();
      stream = std::move(wrapped_stream);
    }

    if (!mongoc_stream_tls_handshake_block(stream.get(), host->host,
                                           connect_timeout_ms, error)) {
      return nullptr;
    }
  }

  // enable read buffering
  UASSERT(stream);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  static_cast<AsyncStream*>(&*stream)->SetCreated();
  return stream.release();
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
AsyncStream::AsyncStream(engine::io::Socket socket) noexcept
    : epoch_(GetNextStreamEpoch()), socket_(std::move(socket)) {
  type = kStreamType;
  destroy = &Destroy;
  close = &Close;
  flush = &Flush;
  writev = &Writev;
  readv = &Readv;
  setsockopt = &Setsockopt;
  get_base_stream = nullptr;
  check_closed = &CheckClosed;
  poll = &Poll;
  failed = &Failed;
  timed_out = &TimedOut;
  should_retry = &ShouldRetry;
}

size_t AsyncStream::BufferedRecv(void* data, size_t size, size_t min_bytes,
                                 engine::Deadline deadline) {
  size_t bytes_stored = 0;
  size_t bytes_left = size;
  char* pos = static_cast<char*>(data);
  try {
    while ((bytes_stored < min_bytes || !bytes_stored) && bytes_left) {
      size_t iter_bytes_stored = 0;
      const size_t old_recv_buffer_bytes_used = recv_buffer_bytes_used_;
      if (recv_buffer_bytes_used_) {
        // has pending data
        UASSERT(recv_buffer_pos_ <= recv_buffer_bytes_used_);
        const auto batch_size =
            std::min(bytes_left, recv_buffer_bytes_used_ - recv_buffer_pos_);

        std::memcpy(pos, recv_buffer_.data() + recv_buffer_pos_, batch_size);
        iter_bytes_stored = batch_size;
        recv_buffer_pos_ += batch_size;

        UASSERT(recv_buffer_pos_ <= recv_buffer_bytes_used_);
        if (recv_buffer_pos_ == recv_buffer_bytes_used_) {
          recv_buffer_pos_ = 0;
          recv_buffer_bytes_used_ = 0;
        }
      } else {
        UASSERT(!recv_buffer_pos_);
        if (bytes_left < recv_buffer_.size()) {
          // no pending data, can be buffered
          recv_buffer_bytes_used_ += socket_.RecvSome(
              recv_buffer_.data(), recv_buffer_.size(), deadline);
          UASSERT(recv_buffer_bytes_used_ <= recv_buffer_.size());
          if (!recv_buffer_bytes_used_) break;  // EOF
        } else {
          // no pending data, will overflow the buffer, stream directly
          const auto batch_size = bytes_left - bytes_left % recv_buffer_.size();
          iter_bytes_stored = socket_.RecvSome(pos, batch_size, deadline);
        }
      }
      UASSERT(iter_bytes_stored ||
              (recv_buffer_bytes_used_ > old_recv_buffer_bytes_used));
      bytes_left -= iter_bytes_stored;
      bytes_stored += iter_bytes_stored;
      pos += iter_bytes_stored;
    }
  } catch (const engine::io::IoTimeout& timeout_ex) {
    // adjust the counter
    throw engine::io::IoTimeout{bytes_stored + timeout_ex.BytesTransferred()};
  }
  return bytes_stored;
}

cdriver::StreamPtr AsyncStream::Create(engine::io::Socket socket) {
  return cdriver::StreamPtr(new AsyncStream(std::move(socket)));
}

void AsyncStream::Destroy(mongoc_stream_t* stream) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto* self = static_cast<AsyncStream*>(stream);
  LOG_TRACE() << "Destroying async stream " << self;

  Close(stream);

  delete self;
}

int AsyncStream::Close(mongoc_stream_t* stream) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto* self = static_cast<AsyncStream*>(stream);
  LOG_TRACE() << "Closing async stream " << self;
  self->is_timed_out_ = false;

  SetWatcher(self->read_watcher_, {});
  SetWatcher(self->write_watcher_, {});
  try {
    self->socket_.Close();
  } catch (const std::exception&) {
    return -1;
  }
  return 0;
}

int AsyncStream::Flush(mongoc_stream_t* stream) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto* self = static_cast<AsyncStream*>(stream);
  LOG_TRACE() << "Flushing async stream " << self;
  self->is_timed_out_ = false;

  return 0;
}

ssize_t AsyncStream::Writev(mongoc_stream_t* stream, mongoc_iovec_t* iov,
                            size_t iovcnt, int32_t timeout_ms) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto* self = static_cast<AsyncStream*>(stream);
  LOG_TRACE() << "Writing to async stream " << self;
  self->is_timed_out_ = false;
  int error = 0;

  const auto deadline = DeadlineFromTimeoutMs(timeout_ms);

  ssize_t bytes_sent = 0;
  try {
    const engine::TaskCancellationBlocker block_cancel{};
    bytes_sent = self->socket_.SendAll(iov, iovcnt, deadline);
  } catch (const engine::io::IoCancelled&) {
    UASSERT_MSG(false,
                "Cancellation is not supported in cdriver implementation");

    if (!bytes_sent) {
      error = EINVAL;
      bytes_sent = -1;
    }
  } catch (const engine::io::IoTimeout& timeout_ex) {
    self->is_timed_out_ = true;
    error = ETIMEDOUT;
    bytes_sent += timeout_ex.BytesTransferred();
  } catch (const engine::io::IoSystemError& io_sys) {
    error = io_sys.Code().value();
    bytes_sent = -1;
  } catch (const engine::io::IoException& io_ex) {
    error = EINVAL;
    bytes_sent = -1;
  }

  if (self->is_created_) {
    tracing::Span* span = tracing::Span::CurrentSpanUnchecked();
    if (span) {
      try {
        span->AddTag(tracing::kPeerAddress,
                     self->socket_.Getpeername().PrimaryAddressString());
      } catch (const std::exception&) {
        // Getpeername() may throw
      }
    }
  }

  // libmongoc expects restored errno
  errno = error;
  return bytes_sent;
}

ssize_t AsyncStream::Readv(mongoc_stream_t* stream, mongoc_iovec_t* iov,
                           size_t iovcnt, size_t min_bytes,
                           int32_t timeout_ms) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto* self = static_cast<AsyncStream*>(stream);
  LOG_TRACE() << "Reading from async stream " << self;
  self->is_timed_out_ = false;
  int error = 0;

  const auto deadline = DeadlineFromTimeoutMs(timeout_ms);

  size_t recvd_total = 0;
  try {
    engine::TaskCancellationBlocker block_cancel;
    size_t curr_iov = 0;
    while (curr_iov < iovcnt && (min_bytes < recvd_total || !recvd_total)) {
      const auto recvd_now =
          self->BufferedRecv(iov[curr_iov].iov_base, iov[curr_iov].iov_len,
                             min_bytes - recvd_total, deadline);
      if (!recvd_now) break;  // EOF
      recvd_total += recvd_now;

      iov[curr_iov].iov_base =
          reinterpret_cast<char*>(iov[curr_iov].iov_base) + recvd_now;
      iov[curr_iov].iov_len -= recvd_now;

      if (!iov[curr_iov].iov_len) ++curr_iov;
    }
  } catch (const engine::io::IoCancelled&) {
    UASSERT_MSG(false,
                "Cancellation is not supported in cdriver implementation");

    if (!recvd_total) error = EINVAL;
  } catch (const engine::io::IoTimeout& timeout_ex) {
    self->is_timed_out_ = true;
    error = ETIMEDOUT;
    recvd_total += timeout_ex.BytesTransferred();
  } catch (const engine::io::IoSystemError& io_sys) {
    error = io_sys.Code().value();
  } catch (const engine::io::IoException& io_ex) {
    error = EINVAL;
  }
  // return value logic from _mongoc_stream_socket_readv
  if (recvd_total < min_bytes) {
    // libmongoc expects restored errno
    errno = error;
    return -1;
  }
  return recvd_total;
}

int AsyncStream::Setsockopt(mongoc_stream_t* stream, int level, int optname,
                            void* optval, mongoc_socklen_t optlen) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto* self = static_cast<AsyncStream*>(stream);
  LOG_TRACE() << "Setting socket option for async stream " << self;
  self->is_timed_out_ = false;

  return ::setsockopt(self->socket_.Fd(), level, optname, optval, optlen);
}

bool AsyncStream::CheckClosed(mongoc_stream_t* base_stream) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto* stream = static_cast<AsyncStream*>(base_stream);
  LOG_TRACE() << "Checking whether async stream is closed";
  stream->is_timed_out_ = false;

  // XXX: mongoc peeks to get ECONNRESET
  return !stream->socket_.IsValid();
}

ssize_t AsyncStream::Poll(mongoc_stream_poll_t* streams, size_t nstreams,
                          int32_t timeout_ms) noexcept {
  LOG_TRACE() << "Polling " << nstreams << " async streams";

  if (!nstreams) return 0;

  if (engine::current_task::ShouldCancel()) {
    // mark all streams as errored out, mongoc tend to ignore poll errors
    LOG_DEBUG()
        << "Should cancel current task. Marking all streams as errored out.";
    for (size_t i = 0; i < nstreams; ++i) streams[i].revents = POLLERR;
    return nstreams;
  }

  const auto deadline = DeadlineFromTimeoutMs(timeout_ms);

  uint64_t current_epoch = 0;
  std::vector<int> stream_fds(nstreams);
  for (size_t i = 0; i < nstreams; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    const auto* stream = static_cast<const AsyncStream*>(streams[i].stream);
    current_epoch = std::max(current_epoch, stream->epoch_);
    stream_fds[i] = stream->socket_.Fd();
  }
  auto& poller = poller_dispenser->Get(current_epoch);

  for (size_t i = 0; i < nstreams; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto* stream = static_cast<AsyncStream*>(streams[i].stream);
    if (streams[i].events & POLLOUT) {
      SetWatcher(stream->write_watcher_, poller.AddWrite(stream_fds[i]));
    } else if (streams[i].events) {
      SetWatcher(stream->read_watcher_, poller.AddRead(stream_fds[i]));
    }
    streams[i].revents = 0;
  }

  ssize_t ready = 0;
  try {
    AsyncStreamPoller::Event poller_event;
    for (bool has_more = poller.NextEvent(poller_event, deadline); has_more;
         has_more = poller.NextEventNoblock(poller_event)) {
      for (size_t i = 0; i < nstreams; ++i) {
        if (stream_fds[i] == poller_event.fd) {
          ready += !streams[i].revents;
          switch (poller_event.type) {
            case AsyncStreamPoller::Event::kError:
              streams[i].revents |= POLLERR;
              break;
            case AsyncStreamPoller::Event::kRead:
              streams[i].revents |= streams[i].events & POLLIN;
              break;
            case AsyncStreamPoller::Event::kWrite:
              streams[i].revents |= streams[i].events & POLLOUT;
              break;
          }
          break;
        }
      }
    }
  } catch (const std::exception&) {
    errno = EINVAL;
    return -1;
  }

  return ready;
}

void AsyncStream::Failed(mongoc_stream_t* stream) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto* self = static_cast<AsyncStream*>(stream);
  LOG_TRACE() << "Failing async stream " << self;

  Destroy(self);
}

bool AsyncStream::TimedOut(mongoc_stream_t* stream) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto* self = static_cast<AsyncStream*>(stream);
  LOG_TRACE() << "Checking timeout state for async stream " << self;

  return self->is_timed_out_;
}

bool AsyncStream::ShouldRetry(mongoc_stream_t*) noexcept {
  // we handle socket retries ourselves
  return false;
}

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
