#pragma once

#include <sys/socket.h>
#include <sys/un.h>

#include <logging/logging_test.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

inline std::shared_ptr<logging::impl::TpLogger> MakeSocketLogger(
    const std::string& logger_name, std::string filename,
    logging::Format format) {
  auto sink =
      std::make_shared<logging::impl::UnixSocketSink>(std::move(filename));
  return MakeLoggerFromSink(logger_name, sink, format);
}

class SocketLoggingTest : public DefaultLoggerFixture {
 public:
  std::string NextLoggedText() {
    logging::LogFlush();
    return ParseLoggedText(NextReceivedMessage(), logging::Format::kTskv);
  }

  std::string NextReceivedMessage() {
    std::string result;
    if (!messages_consumer_.Pop(
            result, engine::Deadline::FromDuration(utest::kMaxTestWaitTime))) {
      throw std::runtime_error("Timed out while waiting for the next message");
    }
    return result;
  }

 protected:
  SocketLoggingTest()
      : socket_file_(fs::blocking::TempFile::Create()),
        socket_(engine::io::AddrDomain::kUnix, engine::io::SocketType::kStream),
        messages_(MessagesQueue::Create()),
        messages_consumer_(messages_->GetConsumer()) {
    SetUpSocketFile();
    SetUpLogger();
    StartServer();
  }

  ~SocketLoggingTest() override { server_task_.SyncCancel(); }

 private:
  using MessagesQueue = concurrent::SpscQueue<std::string>;

  void RunServer() {
    engine::io::Socket connection = socket_.Accept({});
    const auto producer = messages_->GetProducer();
    std::string buffer(kBufferSize, '\0');

    while (!engine::current_task::ShouldCancel()) {
      const auto bytes_received =
          connection.RecvSome(buffer.data(), buffer.size(), {});
      if (bytes_received == 0) break;

      const bool success = producer.Push(buffer.substr(0, bytes_received));
      UINVARIANT(success, "Failed to push a message");
    }
  }

  void SetUpSocketFile() {
    fs::blocking::RemoveSingleFile(socket_file_.GetPath());

    struct sockaddr_un addr {};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_file_.GetPath().data(),
                 socket_file_.GetPath().size());

    socket_.Bind(engine::io::Sockaddr(static_cast<const void*>(&addr)));

    socket_.Listen();
  }

  void StartServer() {
    server_task_ =
        utils::CriticalAsync("log-server", &SocketLoggingTest::RunServer, this);
  }

  void SetUpLogger() {
    const auto logger = MakeSocketLogger(
        "socket_logger", socket_file_.GetPath(), logging::Format::kTskv);
    // Discard logs from the network interaction.
    logger->SetLevel(logging::Level::kError);
    SetDefaultLogger(logger);
  }

  static constexpr std::size_t kBufferSize = 4096;

  fs::blocking::TempFile socket_file_;
  engine::io::Socket socket_;
  std::shared_ptr<MessagesQueue> messages_;
  MessagesQueue::Consumer messages_consumer_;
  engine::TaskWithResult<void> server_task_;
};

USERVER_NAMESPACE_END
