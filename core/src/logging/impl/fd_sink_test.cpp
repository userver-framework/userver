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
    EXPECT_NO_THROW(sink.Log({"message\n", logging::Level::kWarning}));
  }

  EXPECT_THAT(fs::blocking::ReadFileContents(file_scope.GetPath()),
              testing::HasSubstr("message"));
}

UTEST(FdSink, PipeSinkLog) {
  engine::io::Pipe fd_pipe{};

  auto read_task = engine::AsyncNoSpan([&fd_pipe] {
    const auto result = test::ReadFromFd(
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.reader.Release()));
    EXPECT_EQ(result, test::Messages("message"));
  });
  {
    auto sink = logging::impl::FdSink{
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.writer.Release())};
    EXPECT_NO_THROW(sink.Log({"message\n", logging::Level::kWarning}));
  }
  read_task.Get();
}

UTEST(FdSink, PipeSinkLogStringView) {
  engine::io::Pipe fd_pipe{};

  auto read_task = engine::AsyncNoSpan([&fd_pipe] {
    const auto result = test::ReadFromFd(
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.reader.Release()));
    EXPECT_EQ(result, test::Messages("big message"));
  });
  {
    auto sink = logging::impl::FdSink{
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.writer.Release())};

    const char* message = "BIG MESSAGE NO DATA";
    std::string_view message_str{message, 11};
    EXPECT_NO_THROW(sink.Log({message_str, logging::Level::kWarning}));
  }
  read_task.Get();
}

UTEST(FdSink, PipeSinkLogMulti) {
  engine::io::Pipe fd_pipe{};

  auto read_task = engine::AsyncNoSpan([&fd_pipe] {
    const auto result = test::ReadFromFd(
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.reader.Release()));
    EXPECT_EQ(result, test::Messages("message", "message 2", "message 3"));
  });
  {
    auto sink = logging::impl::FdSink{
        fs::blocking::FileDescriptor::AdoptFd(fd_pipe.writer.Release())};
    EXPECT_NO_THROW(sink.Log({"message\n", logging::Level::kWarning}));
    EXPECT_NO_THROW(sink.Log({"message 2\n", logging::Level::kInfo}));
    EXPECT_NO_THROW(sink.Log({"message 3\n", logging::Level::kCritical}));
  }
  read_task.Get();
}

USERVER_NAMESPACE_END
