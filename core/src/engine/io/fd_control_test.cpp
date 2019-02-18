#include <gtest/gtest.h>

#include <unistd.h>

#include <array>
#include <cerrno>

#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <utest/utest.hpp>
#include <utils/check_syscall.hpp>

#include "fd_control.hpp"

namespace {

class Pipe {
 public:
  Pipe() { utils::CheckSyscall(::pipe(fd_), "creating pipe"); }
  ~Pipe() {
    ::close(fd_[0]);
    ::close(fd_[1]);
  }

  int In() { return fd_[0]; }
  int Out() { return fd_[1]; }

 private:
  int fd_[2];
};

bool HasTimedOut() {
  return engine::current_task::GetCurrentTaskContext()->GetWakeupSource() ==
         engine::impl::TaskContext::WakeupSource::kDeadlineTimer;
}

void CheckedWrite(int fd, const void* buf, size_t len) {
  ASSERT_EQ(len, ::write(fd, buf, len));
}

namespace io = engine::io;
using Deadline = engine::Deadline;
using FdControl = io::impl::FdControl;

}  // namespace

TEST(FdControl, Close) {
  RunInCoro([] {
    Pipe pipe;

    auto read_control = FdControl::Adopt(pipe.In());
    read_control->Close();
    EXPECT_EQ(-1, ::close(pipe.In()));
    EXPECT_EQ(EBADF, errno);
    EXPECT_EQ(0, ::close(pipe.Out()));
  });
}

TEST(FdControl, Ownership) {
  RunInCoro([] {
    Pipe pipe;

    {
      auto read_control = FdControl::Adopt(pipe.In());
      auto write_control = FdControl::Adopt(pipe.Out());
      EXPECT_EQ(pipe.In(), read_control->Fd());
      EXPECT_EQ(pipe.Out(), write_control->Fd());
      write_control->Invalidate();
    }

    EXPECT_EQ(-1, ::close(pipe.In()));
    EXPECT_EQ(EBADF, errno);
    EXPECT_EQ(0, ::close(pipe.Out()));
  });
}

TEST(FdControl, Wait) {
  RunInCoro([] {
    Pipe pipe;
    std::array<char, 16> buf;

    auto read_control = FdControl::Adopt(pipe.In());
    auto& read_dir = read_control->Read();
    EXPECT_FALSE(
        read_dir.Wait(Deadline::FromDuration(std::chrono::milliseconds(100))));
    EXPECT_TRUE(HasTimedOut());

    CheckedWrite(pipe.Out(), buf.data(), 1);
    EXPECT_TRUE(
        read_dir.Wait(Deadline::FromDuration(std::chrono::milliseconds(100))));
    EXPECT_FALSE(HasTimedOut());

    EXPECT_EQ(::read(pipe.In(), buf.data(), buf.size()), 1);
    EXPECT_FALSE(
        read_dir.Wait(Deadline::FromDuration(std::chrono::milliseconds(100))));
    EXPECT_TRUE(HasTimedOut());
  });
}

TEST(FdControl, PartialTransfer) {
  RunInCoro([] {
    Pipe pipe;

    auto read_control = FdControl::Adopt(pipe.In());
    auto& read_dir = read_control->Read();

    std::array<char, 16> buf;
    io::impl::Direction::Lock lock(read_dir);
    try {
      read_dir.PerformIo(lock, &::read, buf.data(), buf.size(),
                         io::impl::TransferMode::kPartial,
                         Deadline::FromDuration(std::chrono::milliseconds(10)),
                         "reading");
      FAIL() << "Did not time out";
    } catch (const io::IoTimeout& ex) {
      EXPECT_EQ(0, ex.BytesTransferred());
    }

    CheckedWrite(pipe.Out(), "test", 4);
    EXPECT_EQ(
        4, read_dir.PerformIo(lock, &::read, buf.data(), buf.size(),
                              io::impl::TransferMode::kPartial, {}, "reading"));
  });
}

TEST(FdControl, WholeTransfer) {
  RunInCoro(
      [] {
        Pipe pipe;

        auto read_control = FdControl::Adopt(pipe.In());
        auto& read_dir = read_control->Read();

        std::array<char, 16> buf;
        io::impl::Direction::Lock lock(read_dir);
        try {
          read_dir.PerformIo(
              lock, &::read, buf.data(), buf.size(),
              io::impl::TransferMode::kWhole,
              Deadline::FromDuration(std::chrono::milliseconds(10)), "reading");
          FAIL() << "Did not time out";
        } catch (const io::IoTimeout& ex) {
          EXPECT_EQ(0, ex.BytesTransferred());
        }

        CheckedWrite(pipe.Out(), "test", 4);
        try {
          read_dir.PerformIo(
              lock, &::read, buf.data(), buf.size(),
              io::impl::TransferMode::kWhole,
              Deadline::FromDuration(std::chrono::milliseconds(10)), "reading");
          FAIL() << "Did not time out";
        } catch (const io::IoTimeout& ex) {
          EXPECT_EQ(4, ex.BytesTransferred());
        }

        CheckedWrite(pipe.Out(), "testtesttesttesttest", 20);
        EXPECT_EQ(buf.size(),
                  read_dir.PerformIo(
                      lock, &::read, buf.data(), buf.size(),
                      io::impl::TransferMode::kWhole,
                      Deadline::FromDuration(std::chrono::milliseconds(250)),
                      "reading"));

        auto sender = engine::impl::Async([fd = pipe.Out()] {
          engine::SleepFor(std::chrono::milliseconds(20));
          CheckedWrite(fd, "test", 4);
          engine::SleepFor(std::chrono::milliseconds(100));
          CheckedWrite(fd, "testtesttesttesttest", 20);
        });
        size_t size;
        EXPECT_NO_THROW(
            size = read_dir.PerformIo(
                lock, &::read, buf.data(), buf.size(),
                io::impl::TransferMode::kWhole,
                Deadline::FromDuration(std::chrono::milliseconds(250)),
                "reading"));
        EXPECT_EQ(buf.size(), size);
      },
      2);
}
