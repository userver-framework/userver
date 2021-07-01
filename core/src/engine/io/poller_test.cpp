#include "poller.hpp"

#include <unistd.h>
#include <array>
#include <cerrno>

#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <utest/utest.hpp>
#include <utils/check_syscall.hpp>

namespace {

class Pipe final {
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

void WriteOne(int fd) {
  std::array<char, 1> buf{'1'};
  ASSERT_EQ(buf.size(), ::write(fd, buf.data(), buf.size()));
}

void ReadOne(int fd) {
  std::array<char, 1> buf{};
  ASSERT_EQ(buf.size(), ::read(fd, buf.data(), buf.size()));
  ASSERT_EQ(buf[0], '1');
}

namespace io = engine::io;
using Deadline = engine::Deadline;
using Poller = io::Poller;

constexpr std::chrono::milliseconds kReadTimeout = kMaxTestWaitTime;
constexpr unsigned kRepetitions = 1000;

}  // namespace

UTEST(Poller, Ctr) { Poller poller; }

UTEST(Poller, ReadEvent) {
  Pipe pipe;
  Poller poller;
  auto watcher = poller.AddRead(pipe.In());

  Poller::Event event{};
  WriteOne(pipe.Out());
  EXPECT_TRUE(
      poller.NextEvent(event, engine::Deadline::FromDuration(kReadTimeout)));
  EXPECT_EQ(event.type, Poller::Event::kRead);
  EXPECT_EQ(event.fd, pipe.In());
  ReadOne(pipe.In());
}

UTEST(Poller, TimedOutReadEvent) {
  Pipe pipe;
  Poller poller;
  auto watcher = poller.AddRead(pipe.In());

  Poller::Event event{};
  EXPECT_FALSE(poller.NextEvent(event, engine::Deadline::Passed()));
}

UTEST(Poller, WriteEvent) {
  Pipe pipe;
  Poller poller;
  auto watcher = poller.AddWrite(pipe.Out());

  Poller::Event event{};
  const bool res = poller.NextEvent(event, engine::Deadline::Passed());
  if (res) {
    EXPECT_EQ(event.type, Poller::Event::kWrite);
    EXPECT_EQ(event.fd, pipe.Out());
  }
}

UTEST(Poller, DestroyActiveReadEvent) {
  Pipe pipe;
  Poller poller;
  auto watcher = poller.AddRead(pipe.In());

  WriteOne(pipe.Out());

  poller.Reset();
}

UTEST(Poller, ResetActiveReadEvent) {
  Pipe pipe;
  Poller poller;
  auto watcher = poller.AddRead(pipe.In());

  WriteOne(pipe.Out());

  poller.Reset();
}

UTEST(Poller, ReadWriteAsync) {
  Pipe pipe;
  Poller poller;
  auto watcher = poller.AddRead(pipe.In());

  auto task = engine::impl::Async([&]() {
    Poller::Event event{};
    EXPECT_TRUE(
        poller.NextEvent(event, engine::Deadline::FromDuration(kReadTimeout)));
    EXPECT_EQ(event.type, Poller::Event::kRead);
    EXPECT_EQ(event.fd, pipe.In());
    ReadOne(pipe.In());
  });

  engine::Yield();
  WriteOne(pipe.Out());
  task.Get();
}

UTEST(Poller, ReadWriteTorture) {
  Pipe pipe;

  for (unsigned i = 0; i < kRepetitions; ++i) {
    auto task = engine::impl::Async([&]() {
      Poller poller;
      auto watcher = poller.AddRead(pipe.In());
      Poller::Event event{};
      EXPECT_TRUE(poller.NextEvent(
          event, engine::Deadline::FromDuration(kReadTimeout)));
      EXPECT_EQ(event.type, Poller::Event::kRead);
      EXPECT_EQ(event.fd, pipe.In());
      ReadOne(pipe.In());
    });

    engine::Yield();
    WriteOne(pipe.Out());
    task.Get();
  }
}

UTEST(Poller, ReadWriteMultipleTorture) {
  constexpr unsigned kPipesCount = 20;
  Pipe pipes[kPipesCount];
  bool pipes_read_from[kPipesCount] = {false};

  for (unsigned i = 0; i < kRepetitions; ++i) {
    auto task = engine::impl::Async([&]() {
      Poller poller;
      std::vector<Poller::WatcherPtr> watchers;
      for (auto& pipe : pipes) watchers.push_back(poller.AddRead(pipe.In()));

      Poller::Event event{};

      for (unsigned i = 0; i < std::size(pipes); ++i) {
        EXPECT_TRUE(poller.NextEvent(
            event, engine::Deadline::FromDuration(kReadTimeout)));
        EXPECT_EQ(event.type, Poller::Event::kRead);

        const auto it = std::find_if(
            std::begin(pipes), std::end(pipes),
            [&event](auto& pipe) { return pipe.In() == event.fd; });
        EXPECT_NE(it, std::end(pipes));
        pipes_read_from[it - std::begin(pipes)] = true;
        ReadOne(event.fd);
      }
    });

    engine::Yield();
    for (auto& pipe : pipes) WriteOne(pipe.Out());
    task.Get();

    for (unsigned i = 0; i < std::size(pipes_read_from); ++i) {
      EXPECT_TRUE(pipes_read_from[i]) << "at " << i;
    }
  }
}
