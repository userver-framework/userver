#include "fd_sink.hpp"

#include <gmock/gmock.h>

#include <userver/engine/async.hpp>
#include <userver/engine/io/pipe.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/utest/utest.hpp>

#include "sink_helper_test.hpp"

USERVER_NAMESPACE_BEGIN

UTEST(FdSink, UnownedSinkLog) {
  const auto file_scope = fs::blocking::TempFile::Create();

  {
    auto fd = fs::blocking::FileDescriptor::Open(
        file_scope.GetPath(), fs::blocking::OpenFlag::kWrite);

    auto sink = logging::impl::UnownedFdSink(fd.GetNative());
    EXPECT_NO_THROW(sink.Log({"default", spdlog::level::warn, "message"}));
  }

  EXPECT_THAT(fs::blocking::ReadFileContents(file_scope.GetPath()),
              testing::HasSubstr("message"));
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
    EXPECT_NO_THROW(sink.Log({"default", spdlog::level::warn, "message"}));
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
    EXPECT_NO_THROW(sink.Log({"default", spdlog::level::warn, message_str}));
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
    EXPECT_NO_THROW(sink.Log({"default", spdlog::level::warn, "message"}));
    EXPECT_NO_THROW(sink.Log({"basic", spdlog::level::info, "message 2"}));
    EXPECT_NO_THROW(
        sink.Log({"current", spdlog::level::critical, "message 3"}));
  }
  read_task.Get();
}

USERVER_NAMESPACE_END
