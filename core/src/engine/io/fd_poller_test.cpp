#include <userver/utest/utest.hpp>

#include <userver/engine/io/fd_poller.hpp>
#include <userver/engine/wait_any.hpp>
#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class Pipe final {
 public:
  Pipe() { utils::CheckSyscall(::pipe(fd_), "creating pipe"); }
  ~Pipe() {
    if (fd_[0] != -1) ::close(fd_[0]);
    if (fd_[1] != -1) ::close(fd_[1]);
  }

  int ExtractIn() { return std::exchange(fd_[0], -1); }
  int ExtractOut() { return std::exchange(fd_[1], -1); }

  int In() { return fd_[0]; }
  int Out() { return fd_[1]; }

 private:
  int fd_[2]{};
};

void CheckedWrite(int fd, const void* buf, size_t len) {
  ASSERT_EQ(len, ::write(fd, buf, len));
}

void CheckedRead(int fd, char* buf, size_t len) {
  ASSERT_EQ(len, ::read(fd, buf, len));
}

const auto kSmallWaitTime = std::chrono::milliseconds(100);

}  // namespace

UTEST(FdPoller, WaitAnyRead) {
  Pipe pipe;
  engine::io::FdPoller poller_read;
  poller_read.Reset(pipe.In(), engine::io::FdPoller::Kind::kRead);

  engine::io::FdPoller poller_write;
  poller_write.Reset(pipe.Out(), engine::io::FdPoller::Kind::kWrite);

  // Cannot read from an empty pipe
  auto num = engine::WaitAnyFor(kSmallWaitTime, poller_read);
  EXPECT_EQ(num, std::nullopt);

  // Can write to an empty pipe
  num = engine::WaitAnyFor(kSmallWaitTime, poller_write);
  EXPECT_EQ(num, 0);

  char buf[] = {1};
  CheckedWrite(pipe.Out(), buf, sizeof(buf));

  // Can read from a non-empty pipe
  num = engine::WaitAnyFor(kSmallWaitTime, poller_read);
  EXPECT_EQ(num, 0);

  CheckedRead(pipe.In(), buf, sizeof(buf));
  poller_read.ResetReady();

  // Cannot read from an empty pipe
  num = engine::WaitAnyFor(kSmallWaitTime, poller_read);
  EXPECT_EQ(num, std::nullopt);
}

USERVER_NAMESPACE_END
