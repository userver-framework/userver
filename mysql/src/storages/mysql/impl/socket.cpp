#include <storages/mysql/impl/socket.hpp>

#include <storages/mysql/impl/mariadb_include.hpp>

#include <userver/storages/mysql/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace {

engine::io::FdPoller::Kind ToUserverEvents(int mysql_events) {
  const bool wants_write = (mysql_events & MYSQL_WAIT_WRITE);
  const bool wants_read =
      ((mysql_events & MYSQL_WAIT_READ) | (mysql_events & MYSQL_WAIT_EXCEPT));

  using Kind = engine::io::FdPoller::Kind;

  if (wants_read && wants_write) {
    return Kind::kReadWrite;
  }
  if (wants_read) {
    return Kind::kRead;
  }
  if (wants_write) {
    return Kind::kWrite;
  }

  UINVARIANT(false, "Failed to recognize events that mysql wants to wait for.");
}

int ToMySQLEvents(engine::io::FdPoller::Kind kind) {
  using Kind = engine::io::FdPoller::Kind;

  switch (kind) {
    case Kind::kReadWrite:
      return MYSQL_WAIT_READ | MYSQL_WAIT_WRITE;
    case Kind::kRead:
      return MYSQL_WAIT_READ;
    case Kind::kWrite:
      return MYSQL_WAIT_WRITE;
  }
}

}  // namespace

Socket::Socket(int fd, int mysql_events)
    : fd_{fd}, mysql_events_to_wait_on_{mysql_events} {}

// Poller is Reset + Invalidated transactionally in Wait, so we don't need to
// invalidate it in destructor.
Socket::~Socket() { UASSERT(!poller_.IsValid()); }

void Socket::SetFd(int fd) { fd_ = fd; }

bool Socket::IsValid() const { return fd_ != -1; }

bool Socket::ShouldWait() const { return mysql_events_to_wait_on_ != 0; }

int Socket::Wait(engine::Deadline deadline) {
  UASSERT(mysql_events_to_wait_on_ != 0);
  UASSERT(IsValid());

  poller_.Reset(fd_, ToUserverEvents(mysql_events_to_wait_on_));

  const auto wait_result = poller_.Wait(deadline);
  Reset();

  if (!wait_result.has_value()) {
    throw MySQLIOException{
        0, "Canceled or timed out while waiting for I/O event to occur"};
  } else {
    mysql_events_to_wait_on_ = 0;
    return ToMySQLEvents(*wait_result);
  }
}

void Socket::SetEvents(int mysql_events) {
  mysql_events_to_wait_on_ = mysql_events;
}

void Socket::Reset() {
  poller_.Invalidate();
  mysql_events_to_wait_on_ = 0;
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
