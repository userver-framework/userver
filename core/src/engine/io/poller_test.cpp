#include "poller.hpp"

#include <sys/param.h>
#include <unistd.h>

#include <array>
#include <cerrno>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/internal/net/net_listener.hpp>
#include <userver/utest/utest.hpp>
#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

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
  int fd_[2]{};
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
using TcpListener = internal::net::TcpListener;

constexpr auto kReadTimeout = utest::kMaxTestWaitTime;
constexpr auto kFailTimeout = std::chrono::milliseconds{100};
constexpr unsigned kRepetitions = 1000;

}  // namespace

UTEST(Poller, Ctr) { Poller poller; }

UTEST(Poller, ReadEvent) {
  Pipe pipe;
  Poller poller;
  poller.Add(pipe.In(), Poller::Event::kRead);

  Poller::Event event{};
  WriteOne(pipe.Out());
  ASSERT_EQ(
      poller.NextEvent(event, engine::Deadline::FromDuration(kReadTimeout)),
      Poller::Status::kSuccess);
  EXPECT_EQ(event.type, Poller::Event::kRead);
  EXPECT_EQ(event.fd, pipe.In());
  ASSERT_EQ(poller.NextEventNoblock(event), Poller::Status::kNoEvents);
  ReadOne(pipe.In());
}

UTEST(Poller, TimedOutReadEvent) {
  Pipe pipe;
  Poller poller;
  poller.Add(pipe.In(), Poller::Event::kRead);

  Poller::Event event{};
  EXPECT_EQ(
      poller.NextEvent(event, engine::Deadline::FromDuration(kFailTimeout)),
      Poller::Status::kNoEvents);
}

UTEST(Poller, EventsAreOneshot) {
  Pipe pipe;
  Poller poller;
  poller.Add(pipe.In(), Poller::Event::kRead);

  Poller::Event event{};
  WriteOne(pipe.Out());
  ASSERT_EQ(
      poller.NextEvent(event, engine::Deadline::FromDuration(kReadTimeout)),
      Poller::Status::kSuccess);
  EXPECT_EQ(event.type, Poller::Event::kRead);
  EXPECT_EQ(event.fd, pipe.In());
  ASSERT_EQ(poller.NextEventNoblock(event), Poller::Status::kNoEvents);
  ReadOne(pipe.In());
  WriteOne(pipe.Out());
  EXPECT_EQ(
      poller.NextEvent(event, engine::Deadline::FromDuration(kFailTimeout)),
      Poller::Status::kNoEvents);
}

UTEST(Poller, WriteEvent) {
  // With pipes this test is unstable on some systems
  TcpListener listener;
  auto socket_pair = listener.MakeSocketPair(
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime));
  Poller poller;
  poller.Add(socket_pair.first.Fd(), Poller::Event::kWrite);

  Poller::Event event{};
  EXPECT_EQ(
      poller.NextEvent(event, engine::Deadline::FromDuration(kReadTimeout)),
      Poller::Status::kSuccess);
  EXPECT_EQ(event.type, Poller::Event::kWrite);
  EXPECT_EQ(event.fd, socket_pair.first.Fd());
  ASSERT_EQ(poller.NextEventNoblock(event), Poller::Status::kNoEvents);
}

UTEST(Poller, ReadWriteEvent) {
  TcpListener listener;
  auto socket_pair = listener.MakeSocketPair(
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime));
  WriteOne(socket_pair.second.Fd());

  Poller poller;
  Poller::Event event{};
  // ensure we will get read readiness
  poller.Add(socket_pair.first.Fd(), Poller::Event::kRead);
  EXPECT_EQ(
      poller.NextEvent(event, engine::Deadline::FromDuration(kReadTimeout)),
      Poller::Status::kSuccess);
  EXPECT_EQ(event.type, Poller::Event::kRead);
  EXPECT_EQ(event.fd, socket_pair.first.Fd());
  ASSERT_EQ(poller.NextEventNoblock(event), Poller::Status::kNoEvents);

  poller.Add(socket_pair.first.Fd(),
             {Poller::Event::kRead, Poller::Event::kWrite});
  EXPECT_EQ(
      poller.NextEvent(event, engine::Deadline::FromDuration(kReadTimeout)),
      Poller::Status::kSuccess);
  EXPECT_TRUE(event.type & Poller::Event::kRead);
  EXPECT_TRUE(event.type & Poller::Event::kWrite);
  EXPECT_EQ(event.fd, socket_pair.first.Fd());
  ASSERT_EQ(poller.NextEventNoblock(event), Poller::Status::kNoEvents);
}

UTEST(Poller, DestroyActiveReadEvent) {
  Pipe pipe;
  Poller poller;
  poller.Add(pipe.In(), Poller::Event::kRead);
  WriteOne(pipe.Out());
  engine::SleepFor(kFailTimeout);
}

UTEST(Poller, ReadWriteAsync) {
  Pipe pipe;
  Poller poller;
  poller.Add(pipe.In(), Poller::Event::kRead);

  auto task = engine::AsyncNoSpan([&]() {
    Poller::Event event{};
    ASSERT_EQ(
        poller.NextEvent(event, engine::Deadline::FromDuration(kReadTimeout)),
        Poller::Status::kSuccess);
    EXPECT_EQ(event.type, Poller::Event::kRead);
    EXPECT_EQ(event.fd, pipe.In());
    ASSERT_EQ(poller.NextEventNoblock(event), Poller::Status::kNoEvents);
    ReadOne(pipe.In());
  });

  engine::Yield();
  WriteOne(pipe.Out());
  task.Get();
}

UTEST(Poller, ReadWriteTorture) {
  Pipe pipe;

  for (unsigned i = 0; i < kRepetitions; ++i) {
    auto task = engine::AsyncNoSpan([&]() {
      Poller poller;
      poller.Add(pipe.In(), Poller::Event::kRead);
      Poller::Event event{};
      ASSERT_EQ(
          poller.NextEvent(event, engine::Deadline::FromDuration(kReadTimeout)),
          Poller::Status::kSuccess);
      EXPECT_EQ(event.type, Poller::Event::kRead);
      EXPECT_EQ(event.fd, pipe.In());
      ASSERT_EQ(poller.NextEventNoblock(event), Poller::Status::kNoEvents);
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
    auto task = engine::AsyncNoSpan([&]() {
      Poller poller;
      for (auto& pipe : pipes) poller.Add(pipe.In(), Poller::Event::kRead);

      Poller::Event event{};

      for (unsigned i = 0; i < std::size(pipes); ++i) {
        ASSERT_EQ(poller.NextEvent(
                      event, engine::Deadline::FromDuration(kReadTimeout)),
                  Poller::Status::kSuccess);
        EXPECT_EQ(event.type, Poller::Event::kRead);

        const auto* it = std::find_if(
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

// Disabled for mac, see https://st.yandex-team.ru/TAXICOMMON-4196
#ifdef BSD
UTEST(Poller, DISABLED_AwaitedEventsChange) {
#else
UTEST(Poller, AwaitedEventsChange) {
#endif
  TcpListener listener;
  auto socket_pair = listener.MakeSocketPair(
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime));

  Poller poller;
  Poller::Event event{};

  poller.Add(socket_pair.first.Fd(), Poller::Event::kWrite);
  ASSERT_EQ(
      poller.NextEvent(event, engine::Deadline::FromDuration(kReadTimeout)),
      Poller::Status::kSuccess);
  EXPECT_EQ(event.type, Poller::Event::kWrite);
  EXPECT_EQ(event.fd, socket_pair.first.Fd());

  poller.Add(socket_pair.first.Fd(),
             {Poller::Event::kRead, Poller::Event::kWrite});
  engine::SleepFor(std::chrono::milliseconds{100});
  poller.Add(socket_pair.first.Fd(), Poller::Event::kRead);
  WriteOne(socket_pair.second.Fd());
  ASSERT_EQ(
      poller.NextEvent(event, engine::Deadline::FromDuration(kReadTimeout)),
      Poller::Status::kSuccess);
  EXPECT_EQ(event.type, Poller::Event::kRead);
  EXPECT_EQ(event.fd, socket_pair.first.Fd());
  ASSERT_EQ(poller.NextEventNoblock(event), Poller::Status::kNoEvents);
  ReadOne(socket_pair.first.Fd());

  poller.Add(socket_pair.first.Fd(), Poller::Event::kWrite);
  WriteOne(socket_pair.second.Fd());
  poller.Add(socket_pair.first.Fd(),
             {Poller::Event::kRead, Poller::Event::kWrite});
  ASSERT_EQ(
      poller.NextEvent(event, engine::Deadline::FromDuration(kReadTimeout)),
      Poller::Status::kSuccess);
  EXPECT_TRUE(event.type & Poller::Event::kRead);
  EXPECT_TRUE(event.type & Poller::Event::kWrite);
  EXPECT_EQ(event.fd, socket_pair.first.Fd());
  ASSERT_EQ(poller.NextEventNoblock(event), Poller::Status::kNoEvents);
  ReadOne(socket_pair.first.Fd());

  poller.Add(socket_pair.first.Fd(),
             {Poller::Event::kRead, Poller::Event::kWrite});
  engine::SleepFor(kFailTimeout);
  WriteOne(socket_pair.second.Fd());
  poller.Add(socket_pair.first.Fd(), Poller::Event::kRead);
  ASSERT_EQ(
      poller.NextEvent(event, engine::Deadline::FromDuration(kReadTimeout)),
      Poller::Status::kSuccess);
  EXPECT_EQ(event.type, Poller::Event::kRead);
  EXPECT_EQ(event.fd, socket_pair.first.Fd());
  ASSERT_EQ(poller.NextEventNoblock(event), Poller::Status::kNoEvents);
}

UTEST(Poller, Interrupt) {
  Pipe pipe;
  Poller poller;

  poller.Add(pipe.In(), Poller::Event::kRead);
  auto task = engine::AsyncNoSpan([&] {
    Poller::Event event{};
    ASSERT_EQ(
        poller.NextEvent(event, engine::Deadline::FromDuration(kReadTimeout)),
        Poller::Status::kInterrupt);
    ASSERT_EQ(poller.NextEventNoblock(event), Poller::Status::kNoEvents);
  });

  engine::Yield();
  poller.Interrupt();
  task.Get();
}

UTEST(Poller, Remove) {
  Pipe pipe;
  Poller poller;
  Poller::Event event{};

  poller.Add(pipe.In(), Poller::Event::kRead);
  WriteOne(pipe.Out());
  engine::SleepFor(kFailTimeout);
  poller.Remove(pipe.In());
  EXPECT_EQ(
      poller.NextEvent(event, engine::Deadline::FromDuration(kFailTimeout)),
      Poller::Status::kNoEvents);
}

USERVER_NAMESPACE_END
