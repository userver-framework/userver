#include "async_stream.hpp"

#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>

#include <cstdlib>
#include <memory>
#include <vector>

#include <boost/lexical_cast.hpp>

#include <engine/async.hpp>
#include <engine/deadline.hpp>
#include <engine/io/addr.hpp>
#include <engine/io/error.hpp>
#include <engine/task/task_with_result.hpp>
#include <logging/log.hpp>

namespace asyncmongo {
namespace {

constexpr int kStreamType = 0x53755459;

void Destroy(mongoc_stream_t* base_stream) {
  auto* stream = static_cast<AsyncStream*>(base_stream);
  LOG_INFO() << "Destroying async stream";
  delete stream;
}

int Close(mongoc_stream_t* base_stream) {
  auto* stream = static_cast<AsyncStream*>(base_stream);
  LOG_INFO() << "Closing async stream";
  stream->timed_out = false;
  try {
    stream->socket.Close();
  } catch (const std::exception&) {
    return -1;
  }
  return 0;
}

int Flush(mongoc_stream_t* base_stream) {
  auto* stream = static_cast<AsyncStream*>(base_stream);
  LOG_INFO() << "Flushing async stream";
  stream->timed_out = false;
  return 0;
}

ssize_t Writev(mongoc_stream_t* base_stream, mongoc_iovec_t* iov, size_t iovcnt,
               int32_t timeout_msec) {
  auto* stream = static_cast<AsyncStream*>(base_stream);
  LOG_INFO() << "Writing to async stream";

  const auto deadline =
      engine::Deadline::FromDuration(std::chrono::milliseconds(timeout_msec));
  stream->timed_out = false;

  ssize_t total_sent = 0;
  try {
    for (size_t i = 0; i < iovcnt; ++i) {
      total_sent +=
          stream->socket.SendAll(iov[i].iov_base, iov[i].iov_len, deadline);
    }
  } catch (const engine::io::IoTimeout&) {
    stream->timed_out = true;
    return -1;
  } catch (const engine::io::IoError&) {
    return -1;
  }
  return total_sent;
}

ssize_t Readv(mongoc_stream_t* base_stream, mongoc_iovec_t* iov, size_t iovcnt,
              size_t min_bytes, int32_t timeout_msec) {
  auto* stream = static_cast<AsyncStream*>(base_stream);
  LOG_INFO() << "Reading from async stream";

  const auto deadline =
      engine::Deadline::FromDuration(std::chrono::milliseconds(timeout_msec));
  stream->timed_out = false;

  size_t total_read = 0;
  try {
    size_t cur_iov = 0;
    while (cur_iov < iovcnt) {
      auto read_bytes = stream->socket.RecvSome(iov[cur_iov].iov_base,
                                                iov[cur_iov].iov_len, deadline);
      total_read += read_bytes;
      if (total_read >= min_bytes) break;

      iov[cur_iov].iov_base =
          reinterpret_cast<char*>(iov[cur_iov].iov_base) + read_bytes;
      iov[cur_iov].iov_len -= read_bytes;
      if (!iov[cur_iov].iov_len) ++cur_iov;
    }
  } catch (const engine::io::IoTimeout&) {
    stream->timed_out = true;
  } catch (const engine::io::IoError&) {
    // pass
  }
  return total_read >= min_bytes ? total_read : -1;
}

int Setsockopt(mongoc_stream_t* base_stream, int level, int optname,
               void* optval, mongoc_socklen_t optlen) {
  auto* stream = static_cast<AsyncStream*>(base_stream);
  LOG_INFO() << "Setting async stream option";
  stream->timed_out = false;
  return ::setsockopt(stream->socket.Fd(), level, optname, optval, optlen);
}

bool CheckClosed(mongoc_stream_t* base_stream) {
  auto* stream = static_cast<AsyncStream*>(base_stream);
  LOG_INFO() << "Checking whether async stream is closed";
  stream->timed_out = false;
  // XXX: mongoc peeks to get ECONNRESET
  return !stream->socket.IsOpen();
}

ssize_t Poll(mongoc_stream_poll_t* streams, size_t nstreams, int32_t timeout) {
  LOG_INFO() << "Polling async streams";

  const auto deadline =
      engine::Deadline::FromDuration(std::chrono::milliseconds(timeout));

  // we need WaitAny for this, really
  std::vector<engine::TaskWithResult<int>> waiters;
  for (size_t i = 0; i < nstreams; ++i) {
    auto* stream = static_cast<AsyncStream*>(streams[i].stream);
    stream->timed_out = false;
    if (streams[i].events & POLLOUT) {
      waiters.push_back(engine::Async([deadline, stream] {
        try {
          stream->socket.WaitWriteable(deadline);
          return POLLOUT;
        } catch (const engine::io::IoTimeout&) {
          stream->timed_out = true;
          return 0;
        } catch (const engine::io::IoError&) {
          return POLLERR;
        }
      }));
    } else {
      waiters.push_back(
          engine::Async([ deadline, stream, events = streams[i].events ] {
            try {
              stream->socket.WaitReadable(deadline);
              return POLLIN & events;
            } catch (const engine::io::IoTimeout&) {
              stream->timed_out = true;
              return 0;
            } catch (const engine::io::IoError&) {
              return POLLERR;
            }
          }));
    }
  }

  ssize_t ready = 0;
  for (size_t i = 0; i < nstreams; ++i) {
    streams[i].revents = waiters[i].Get();
    ready += !!streams[i].revents;
  }
  return ready;
}

void Failed(mongoc_stream_t* base_stream) {
  auto* stream = static_cast<AsyncStream*>(base_stream);
  LOG_INFO() << "Failing async stream";
  stream->timed_out = false;
  Destroy(stream);
}

bool TimedOut(mongoc_stream_t* base_stream) {
  auto* stream = static_cast<AsyncStream*>(base_stream);
  LOG_INFO() << "Checking async stream timeout";
  return stream->timed_out;
}

}  // namespace

AsyncStream::AsyncStream() : timed_out(false) {
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
}

struct AddrinfoDeleter {
  void operator()(struct addrinfo* p) const { freeaddrinfo(p); }
};
using AddrinfoPtr = std::unique_ptr<struct addrinfo, AddrinfoDeleter>;

mongoc_stream_t* AsyncStream::Create(const mongoc_uri_t* uri,
                                     const mongoc_host_list_t* host, void*,
                                     bson_error_t* error) {
  auto stream = std::make_unique<AsyncStream>();

  const auto connect_timeout_ms =
      mongoc_uri_get_option_as_int32(uri, MONGOC_URI_CONNECTTIMEOUTMS, 5000);

  const auto port_string = boost::lexical_cast<std::string>(host->port);

  struct addrinfo hints;
  ::memset(&hints, 0, sizeof(hints));
  hints.ai_family = host->family;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo* ai_result_raw;
  LOG_WARNING() << "Trying to resolve " << host->host_and_port;
  if (getaddrinfo(host->host, port_string.c_str(), &hints, &ai_result_raw)) {
    LOG_ERROR() << "Cannot resolve " << host->host;
    bson_set_error(error, MONGOC_ERROR_STREAM,
                   MONGOC_ERROR_STREAM_NAME_RESOLUTION, "Cannot resolve %s",
                   host->host);
    return nullptr;
  }
  AddrinfoPtr ai_result(ai_result_raw);

  for (auto res = ai_result.get(); res; res = res->ai_next) {
    try {
      stream->socket = engine::io::Connect(
          engine::io::Addr(res->ai_addr, res->ai_socktype, res->ai_protocol),
          engine::Deadline::FromDuration(
              std::chrono::milliseconds(connect_timeout_ms)));
      break;
    } catch (const engine::io::IoError&) {
      // pass
    }
  }

  if (!stream->socket) {
    bson_set_error(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_CONNECT,
                   "Cannot connect to %s", host->host_and_port);
    return nullptr;
  }
  return stream.release();
}

}  // namespace asyncmongo
