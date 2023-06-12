#include "fd_sink.hpp"

#include <userver/engine/async.hpp>
#include <userver/engine/io/pipe.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/text.hpp>

#include "sink_helper_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {
const auto msg_a = std::string(test::kEightMb, 'a');
const auto msg_b = std::string(test::kEightMb, 'b');
const auto msg_c = std::string(test::kEightMb, 'c');
}  // namespace

UTEST(FdSink, StdoutSinkLog) {
  auto sink = logging::impl::StdoutSink();
  EXPECT_NO_THROW(sink.log({"default", spdlog::level::warn, "message"}));
}

UTEST(FdSink, StderrSinkLog) {
  auto sink = logging::impl::StderrSink();
  EXPECT_NO_THROW(sink.log({"default", spdlog::level::critical, "message"}));
}

UTEST(FdSink, PipeSinkLog) {
  engine::io::Pipe fd_pipe{};

  auto read_task = engine::AsyncNoSpan([&fd_pipe] {
    const auto result = test::ReadFromFd(
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.reader.Release()));
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result.front(), "[datetime] [default] [warning] message");
  });
  {
    auto sink = logging::impl::FdSink{
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.writer.Release())};
    EXPECT_NO_THROW(sink.log({"default", spdlog::level::warn, "message"}));
  }
  read_task.Get();
}

UTEST(FdSink, PipeSinkLogStringView) {
  engine::io::Pipe fd_pipe{};

  auto read_task = engine::AsyncNoSpan([&fd_pipe] {
    const auto result = test::ReadFromFd(
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.reader.Release()));
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result.front(), "[datetime] [default] [warning] big message");
  });
  {
    auto sink = logging::impl::FdSink{
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.writer.Release())};

    const char* message = "BIG MESSAGE NO DATA";
    std::string_view message_str{message, 11};
    EXPECT_NO_THROW(sink.log({"default", spdlog::level::warn, message_str}));
  }
  read_task.Get();
}

UTEST(FdSink, PipeSinkLogMulti) {
  engine::io::Pipe fd_pipe{};

  auto read_task = engine::AsyncNoSpan([&fd_pipe] {
    const auto result = test::ReadFromFd(
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.reader.Release()));
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "[datetime] [default] [warning] message");
    EXPECT_EQ(result[1], "[datetime] [basic] [info] message 2");
    EXPECT_EQ(result[2], "[datetime] [current] [critical] message 3");
  });
  {
    auto sink = logging::impl::FdSink{
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.writer.Release())};
    EXPECT_NO_THROW(sink.log({"default", spdlog::level::warn, "message"}));
    EXPECT_NO_THROW(sink.log({"basic", spdlog::level::info, "message 2"}));
    EXPECT_NO_THROW(
        sink.log({"current", spdlog::level::critical, "message 3"}));
  }
  read_task.Get();
}

UTEST_MT(FdSink, PipeSinkLogAsync, 4) {
  engine::io::Pipe fd_pipe{};

  auto read_task = engine::AsyncNoSpan([&fd_pipe] {
    const auto result = test::ReadFromFd(
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.reader.Release()));
    EXPECT_EQ(result.size(), 3);
    auto expect_predicate = [](auto str) {
      return str == "[datetime] [default] [warning] message" ||
             str == "[datetime] [basic] [info] message 2" ||
             str == "[datetime] [current] [critical] message 3";
    };
    EXPECT_PRED1(expect_predicate, result[0]);
    EXPECT_PRED1(expect_predicate, result[1]);
    EXPECT_PRED1(expect_predicate, result[2]);
  });

  {
    auto sink = logging::impl::FdSink{
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.writer.Release())};

    auto log_task_1 = engine::AsyncNoSpan([&sink] {
      EXPECT_NO_THROW(sink.log({"default", spdlog::level::warn, "message"}));
    });

    auto log_task_2 = engine::AsyncNoSpan([&sink] {
      EXPECT_NO_THROW(sink.log({"basic", spdlog::level::info, "message 2"}));
    });

    auto log_task_3 = engine::AsyncNoSpan([&sink] {
      EXPECT_NO_THROW(
          sink.log({"current", spdlog::level::critical, "message 3"}));
    });
    log_task_1.Get();
    log_task_2.Get();
    log_task_3.Get();
  }

  read_task.Get();
}

UTEST_MT(FdSink, PipeSinkLogAsyncBigMessage, 4) {
  engine::io::Pipe fd_pipe{};

  auto read_task = engine::AsyncNoSpan([&fd_pipe] {
    const auto result = test::ReadFromFd(
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.reader.Release()));
    EXPECT_EQ(result.size(), 3);
    auto expect_predicate = [](auto str) {
      return str == "[datetime] [default] [warning] " + msg_a ||
             str == "[datetime] [basic] [info] " + msg_b ||
             str == "[datetime] [current] [critical] " + msg_c;
    };
    EXPECT_PRED1(expect_predicate, result[0]);
    EXPECT_PRED1(expect_predicate, result[1]);
    EXPECT_PRED1(expect_predicate, result[2]);
  });

  {
    auto sink = logging::impl::FdSink{
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.writer.Release())};

    auto log_task_1 = engine::AsyncNoSpan([&sink] {
      EXPECT_NO_THROW(sink.log({"default", spdlog::level::warn, msg_a}));
    });

    auto log_task_2 = engine::AsyncNoSpan([&sink] {
      EXPECT_NO_THROW(sink.log({"basic", spdlog::level::info, msg_b}));
    });

    auto log_task_3 = engine::AsyncNoSpan([&sink] {
      EXPECT_NO_THROW(sink.log({"current", spdlog::level::critical, msg_c}));
    });
    log_task_1.Get();
    log_task_2.Get();
    log_task_3.Get();
  }

  read_task.Get();
}

TEST(FdSink, PipeSinkLogAsyncNoCore) {
  std::array<int, 2> fd_pipe{-1, -1};
#ifdef HAVE_PIPE2
  ::pipe2(fd_pipe.data(), O_NONBLOCK | O_CLOEXEC);
#else
  ::pipe(fd_pipe.data());
#endif
  auto read_fd = fs::blocking::FileDescriptor::AdoptFd(fd_pipe[0]);
  auto write_fd = fs::blocking::FileDescriptor::AdoptFd(fd_pipe[1]);

  auto read_task = std::thread([&read_fd] {
    const auto result = test::ReadFromFd(std::move(read_fd));
    EXPECT_EQ(result.size(), 3);
    auto expect_predicate = [](auto str) {
      return str == "[datetime] [default] [warning] message" ||
             str == "[datetime] [basic] [info] message 2" ||
             str == "[datetime] [current] [critical] message 3";
    };
    EXPECT_PRED1(expect_predicate, result[0]);
    EXPECT_PRED1(expect_predicate, result[1]);
    EXPECT_PRED1(expect_predicate, result[2]);
  });

  {
    auto sink = logging::impl::FdSink{
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe[1])};

    auto log_task_1 = std::thread([&sink] {
      EXPECT_NO_THROW(sink.log({"default", spdlog::level::warn, "message"}));
    });

    auto log_task_2 = std::thread([&sink] {
      EXPECT_NO_THROW(sink.log({"basic", spdlog::level::info, "message 2"}));
    });

    auto log_task_3 = std::thread([&sink] {
      EXPECT_NO_THROW(
          sink.log({"current", spdlog::level::critical, "message 3"}));
    });
    log_task_1.join();
    log_task_2.join();
    log_task_3.join();
  }

  read_task.join();
}

TEST(FdSink, PipeSinkLogAsyncNoCoreBigString) {
  std::array<int, 2> fd_pipe{-1, -1};
#ifdef HAVE_PIPE2
  ::pipe2(fd_pipe.data(), O_NONBLOCK | O_CLOEXEC);
#else
  ::pipe(fd_pipe.data());
#endif
  auto read_fd = fs::blocking::FileDescriptor::AdoptFd(fd_pipe[0]);
  auto write_fd = fs::blocking::FileDescriptor::AdoptFd(fd_pipe[1]);

  auto read_task = std::thread([&read_fd] {
    const auto result = test::ReadFromFd(std::move(read_fd));
    EXPECT_EQ(result.size(), 3);
    auto expect_predicate = [](auto str) {
      return str == "[datetime] [default] [warning] " + msg_a ||
             str == "[datetime] [basic] [info] " + msg_b ||
             str == "[datetime] [current] [critical] " + msg_c;
    };
    EXPECT_PRED1(expect_predicate, result[0]);
    EXPECT_PRED1(expect_predicate, result[1]);
    EXPECT_PRED1(expect_predicate, result[2]);
  });

  {
    auto sink = logging::impl::FdSink{
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe[1])};

    auto log_task_1 = std::thread([&sink] {
      EXPECT_NO_THROW(sink.log({"default", spdlog::level::warn, msg_a}));
    });

    auto log_task_2 = std::thread([&sink] {
      EXPECT_NO_THROW(sink.log({"basic", spdlog::level::info, msg_b}));
    });

    auto log_task_3 = std::thread([&sink] {
      EXPECT_NO_THROW(sink.log({"current", spdlog::level::critical, msg_c}));
    });
    log_task_1.join();
    log_task_2.join();
    log_task_3.join();
  }

  read_task.join();
}

USERVER_NAMESPACE_END
