#include "fd_sink.hpp"

#include <array>
#include <climits>
#include <future>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <unistd.h>

#include <fmt/format.h>

#include <logging/impl/sink_helper_test.hpp>
#include <logging/impl/synchronized_sink.hpp>
#include <logging/tp_logger.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/io/pipe.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text.hpp>
#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// Larger than PIPE_BUF so a single write(2)/writev(2) is not required to be atomic on pipes.
constexpr std::size_t kOverPipeBufSize = PIPE_BUF + 2048;
constexpr std::size_t kMessagesPerWriter = 64;

std::string MakeMarkedRecord(char writer_id, std::size_t index) {
    const auto header = fmt::format("BEGIN_{}_{:05}_", writer_id, index);
    const auto footer = fmt::format("_END_{}_{:05}", writer_id, index);
    std::string record;
    record.reserve(kOverPipeBufSize + 1);
    record = header;
    record.append(kOverPipeBufSize - header.size() - footer.size(), 'x');
    record += footer;
    record.push_back('\n');
    UINVARIANT(record.size() > PIPE_BUF, "Record must exceed PIPE_BUF to cover the tear case");
    return record;
}

bool IsIntactRecord(std::string_view line) {
    if (line.size() < 20) {
        return false;
    }
    if (!line.starts_with("BEGIN_") || line.find("_END_") == std::string_view::npos) {
        return false;
    }
    const auto writer_id = line[6];
    const auto begin_idx = line.substr(8, 5);
    const auto end_marker = fmt::format("_END_{}_{}", writer_id, begin_idx);
    return line.ends_with(end_marker);
}

std::string ReadAllFromFd(fs::blocking::FileDescriptor&& fd) {
    std::string data;
    char buf[4096];
    while (true) {
        const auto read_size = fd.Read(buf);
        if (read_size == 0) {
            break;
        }
        data.append(buf, read_size);
    }
    return data;
}

void ExpectIntactRecords(const std::string& data, std::size_t expected_count) {
    const auto lines = utils::text::Split(data, "\n");
    std::size_t intact = 0;
    for (const auto& line : lines) {
        if (line.empty()) {
            continue;
        }
        EXPECT_TRUE(IsIntactRecord(line)) << "Torn or malformed log line: " << line.substr(0, 64);
        ++intact;
    }
    EXPECT_EQ(intact, expected_count);
}

struct BlockingPipe final {
    BlockingPipe() {
        std::array<int, 2> fds{-1, -1};
        utils::CheckSyscall(::pipe(fds.data()), "creating a blocking pipe");
        reader_fd = fds[0];
        writer_fd = fds[1];
    }

    int reader_fd{-1};
    int writer_fd{-1};
};

}  // namespace

UTEST(FdSink, UnownedSinkLog) {
    const auto file_scope = fs::blocking::TempFile::Create();

    {
        auto fd = fs::blocking::FileDescriptor::Open(file_scope.GetPath(), fs::blocking::OpenFlag::kWrite);

        auto sink = logging::impl::UnownedFdSink(fd.GetNative());
        EXPECT_NO_THROW(sink.Log("message\n"));
    }

    EXPECT_THAT(fs::blocking::ReadFileContents(file_scope.GetPath()), testing::HasSubstr("message"));
}

UTEST(FdSink, PipeSinkLog) {
    engine::io::Pipe fd_pipe{};

    auto read_task = engine::AsyncNoTracing([&fd_pipe] {
        const auto result = test::ReadFromFd(fs::blocking::FileDescriptor::AdoptFd(fd_pipe.reader.Release()));
        EXPECT_EQ(result, test::Messages("message"));
    });
    {
        auto sink = logging::impl::FdSink{fs::blocking::FileDescriptor::AdoptFd(fd_pipe.writer.Release())};
        EXPECT_NO_THROW(sink.Log("message\n"));
    }
    read_task.Get();
}

UTEST(FdSink, PipeSinkLogStringView) {
    engine::io::Pipe fd_pipe{};

    auto read_task = engine::AsyncNoTracing([&fd_pipe] {
        const auto result = test::ReadFromFd(fs::blocking::FileDescriptor::AdoptFd(fd_pipe.reader.Release()));
        EXPECT_EQ(result, test::Messages("big message"));
    });
    {
        auto sink = logging::impl::FdSink{fs::blocking::FileDescriptor::AdoptFd(fd_pipe.writer.Release())};

        const char* message = "BIG MESSAGE NO DATA";
        const std::string_view message_str{message, 11};
        EXPECT_NO_THROW(sink.Log(message_str));
    }
    read_task.Get();
}

UTEST(FdSink, PipeSinkLogMulti) {
    engine::io::Pipe fd_pipe{};

    auto read_task = engine::AsyncNoTracing([&fd_pipe] {
        const auto result = test::ReadFromFd(fs::blocking::FileDescriptor::AdoptFd(fd_pipe.reader.Release()));
        EXPECT_EQ(result, test::Messages("message", "message 2", "message 3"));
    });
    {
        auto sink = logging::impl::FdSink{fs::blocking::FileDescriptor::AdoptFd(fd_pipe.writer.Release())};
        EXPECT_NO_THROW(sink.Log("message\n"));
        EXPECT_NO_THROW(sink.Log("message 2\n"));
        EXPECT_NO_THROW(sink.Log("message 3\n"));
    }
    read_task.Get();
}

// Two sinks / two loggers writing to one pipe is the @stdout multi-logger case.
// Without a shared lock, records larger than PIPE_BUF get torn across lines.
UTEST(SynchronizedSink, ConcurrentLargeWritesKeepRecordIntegrity) {
    BlockingPipe pipe;
    const int writer_fd = pipe.writer_fd;
    const auto write_mutex = std::make_shared<std::mutex>();

    std::string data;
    std::thread reader{[&data, reader_fd = pipe.reader_fd] {
        data = ReadAllFromFd(fs::blocking::FileDescriptor::AdoptFd(reader_fd));
    }};

    {
        logging::impl::SynchronizedSink<logging::impl::UnownedFdSink> sink_a{write_mutex, writer_fd};
        logging::impl::SynchronizedSink<logging::impl::UnownedFdSink> sink_b{write_mutex, writer_fd};

        auto writer_a = std::async(std::launch::async, [&sink_a] {
            for (std::size_t i = 0; i < kMessagesPerWriter; ++i) {
                sink_a.Log(MakeMarkedRecord('A', i));
            }
        });
        auto writer_b = std::async(std::launch::async, [&sink_b] {
            for (std::size_t i = 0; i < kMessagesPerWriter; ++i) {
                sink_b.Log(MakeMarkedRecord('B', i));
            }
        });
        writer_a.get();
        writer_b.get();
    }

    EXPECT_EQ(::close(writer_fd), 0);
    reader.join();
    ExpectIntactRecords(data, 2 * kMessagesPerWriter);
}

UTEST(SynchronizedSink, TwoLoggersToSharedFdKeepRecordIntegrity) {
    BlockingPipe pipe;
    const int writer_fd = pipe.writer_fd;
    const auto write_mutex = std::make_shared<std::mutex>();

    std::string data;
    std::thread reader{[&data, reader_fd = pipe.reader_fd] {
        data = ReadAllFromFd(fs::blocking::FileDescriptor::AdoptFd(reader_fd));
    }};

    {
        logging::impl::TpLogger logger_a{logging::Format::kTskv, "default"};
        logging::impl::TpLogger logger_b{logging::Format::kTskv, "service"};
        logger_a.AddSink(logging::impl::MakeSynchronizedSink<logging::impl::UnownedFdSink>(write_mutex, writer_fd));
        logger_b.AddSink(logging::impl::MakeSynchronizedSink<logging::impl::UnownedFdSink>(write_mutex, writer_fd));
        logger_a.SetLevel(logging::Level::kInfo);
        logger_b.SetLevel(logging::Level::kInfo);

        auto writer_a = std::async(std::launch::async, [&logger_a] {
            for (std::size_t i = 0; i < kMessagesPerWriter; ++i) {
                logging::impl::TextLogItem item;
                item.log_line = MakeMarkedRecord('A', i);
                logger_a.Log(logging::Level::kInfo, item);
            }
        });
        auto writer_b = std::async(std::launch::async, [&logger_b] {
            for (std::size_t i = 0; i < kMessagesPerWriter; ++i) {
                logging::impl::TextLogItem item;
                item.log_line = MakeMarkedRecord('B', i);
                logger_b.Log(logging::Level::kInfo, item);
            }
        });
        writer_a.get();
        writer_b.get();
    }

    EXPECT_EQ(::close(writer_fd), 0);
    reader.join();
    ExpectIntactRecords(data, 2 * kMessagesPerWriter);
}

USERVER_NAMESPACE_END
