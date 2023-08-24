#include <userver/utest/utest.hpp>

#include <array>
#include <userver/utils/async.hpp>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/pipe.hpp>

#include <utils/signal_catcher.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace io = engine::io;
using Deadline = engine::Deadline;

constexpr std::chrono::milliseconds kIoTimeout{10};

}  // namespace

UTEST(Pipe, Smoke) { io::Pipe pipe; }

UTEST(Pipe, Wait) {
  io::Pipe pipe;
  std::array<char, 16> buf{};

  EXPECT_FALSE(pipe.reader.WaitReadable(Deadline::FromDuration(kIoTimeout)));
  EXPECT_TRUE(pipe.writer.WaitWriteable(Deadline::FromDuration(kIoTimeout)));

  ASSERT_EQ(1, pipe.writer.WriteAll(buf.data(), 1,
                                    Deadline::FromDuration(kIoTimeout)));
  EXPECT_TRUE(pipe.reader.WaitReadable(Deadline::FromDuration(kIoTimeout)));

  ASSERT_EQ(1, pipe.reader.ReadAll(buf.data(), 1,
                                   Deadline::FromDuration(kIoTimeout)));
  EXPECT_FALSE(pipe.reader.WaitReadable(Deadline::FromDuration(kIoTimeout)));
}

UTEST(Pipe, Read) {
  io::Pipe pipe;
  std::array<char, 16> buf{};

  UEXPECT_THROW([[maybe_unused]] auto bytes_read = pipe.reader.ReadSome(
                    buf.data(), 1, Deadline::FromDuration(kIoTimeout)),
                io::IoTimeout);

  ASSERT_EQ(
      4, pipe.writer.WriteAll("test", 4, Deadline::FromDuration(kIoTimeout)));

  EXPECT_EQ(4, pipe.reader.ReadSome(buf.data(), buf.size(),
                                    Deadline::FromDuration(kIoTimeout)));
  EXPECT_STREQ("test", buf.data());

  auto reader = engine::AsyncNoSpan([&] {
    return pipe.reader.ReadAll(buf.data(), buf.size(),
                               Deadline::FromDuration(utest::kMaxTestWaitTime));
  });
  reader.WaitFor(kIoTimeout);
  ASSERT_FALSE(reader.IsFinished());
  ASSERT_EQ(
      4, pipe.writer.WriteAll("test", 4, Deadline::FromDuration(kIoTimeout)));
  reader.WaitFor(kIoTimeout);
  ASSERT_FALSE(reader.IsFinished());
  ASSERT_EQ(12, pipe.writer.WriteAll("testtesttest", 12,
                                     Deadline::FromDuration(kIoTimeout)));

  reader.WaitFor(kIoTimeout);
  EXPECT_TRUE(reader.IsFinished());
  EXPECT_EQ(buf.size(), reader.Get());
}

UTEST(Pipe, Write) {
  io::Pipe pipe;
  std::vector<char> buf(1024 * 1024);
  size_t total_wrote_bytes = 0;
  auto writer = engine::AsyncNoSpan([&] {
    try {
      while (true) {
        const auto wrote_bytes = pipe.writer.WriteAll(
            buf.data(), buf.size(), Deadline::FromDuration(kIoTimeout));
        ASSERT_EQ(buf.size(), wrote_bytes);
        total_wrote_bytes += wrote_bytes;
      }
    } catch (const io::IoTimeout& ex) {
      total_wrote_bytes += ex.BytesTransferred();
      throw;
    }
  });
  writer.WaitFor(utest::kMaxTestWaitTime);
  ASSERT_TRUE(writer.IsFinished());
  UEXPECT_THROW(writer.Get(), io::IoTimeout);
  EXPECT_FALSE(pipe.writer.WaitWriteable(Deadline::FromDuration(kIoTimeout)));

  EXPECT_NE(0, total_wrote_bytes);
  while (total_wrote_bytes) {
    const auto read_bytes =
        pipe.reader.ReadAll(buf.data(), std::min(total_wrote_bytes, buf.size()),
                            Deadline::FromDuration(kIoTimeout));
    EXPECT_EQ(read_bytes, std::min(total_wrote_bytes, buf.size()));
    total_wrote_bytes -= read_bytes;
  }
}

namespace {
const utils::SignalCatcher catcher{SIGPIPE};
}

UTEST(Pipe, CloseRead) {
  io::Pipe pipe;
  std::array<char, 16> buf{};

  EXPECT_EQ(1, pipe.writer.WriteAll(buf.data(), 1,
                                    Deadline::FromDuration(kIoTimeout)));
  pipe.reader.Close();
  try {
    [[maybe_unused]] auto wrote_bytes =
        pipe.writer.WriteAll(buf.data(), 1, Deadline::FromDuration(kIoTimeout));
    FAIL() << "Broken pipe undetected";
  } catch (const io::IoSystemError& ex) {
    EXPECT_EQ(EPIPE, ex.Code().value());
  }
}

UTEST(Pipe, CloseWrite) {
  io::Pipe pipe;
  std::array<char, 16> buf{};

  UEXPECT_THROW([[maybe_unused]] auto read_bytes = pipe.reader.ReadAll(
                    buf.data(), 1, Deadline::FromDuration(kIoTimeout)),
                io::IoTimeout);
  pipe.writer.Close();
  EXPECT_EQ(0, pipe.reader.ReadAll(buf.data(), 1,
                                   Deadline::FromDuration(kIoTimeout)));
}

USERVER_NAMESPACE_END
