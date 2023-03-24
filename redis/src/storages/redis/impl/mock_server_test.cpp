#include "mock_server_test.hpp"

#include <sstream>
#include <thread>

#include <boost/algorithm/string.hpp>

#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kCrlf = "\r\n";

void HandlerHotFound(const std::string& command,
                     const std::vector<std::string>& args) {
  std::ostringstream os;
  bool first = true;
  for (const auto& arg : args) {
    if (first)
      first = false;
    else
      os << ' ';
    os << '\'' << arg << '\'';
  }
  ASSERT_TRUE(false) << "Handler not found. command: '" << command
                     << "' args: [" << os.str() << ']';
}

}  // namespace

MockRedisServerBase::MockRedisServerBase(int port)
    : acceptor_(io_service_),
      socket_(io_service_),
      reader_(redisReaderCreate(), &redisReaderFree) {
  acceptor_.open(io::ip::tcp::v4());
  boost::asio::ip::tcp::acceptor::reuse_address option(true);
  acceptor_.set_option(option);
  acceptor_.bind(io::ip::tcp::endpoint(io::ip::tcp::v4(), port));
  acceptor_.listen();

  thread_ = std::thread(&MockRedisServerBase::Work, this);
}

MockRedisServerBase::~MockRedisServerBase() { Stop(); }

void MockRedisServerBase::SendReplyOk(const std::string& reply) {
  SendReply('+' + reply + kCrlf);
}

void MockRedisServerBase::SendReplyError(const std::string& reply) {
  SendReply('-' + reply + kCrlf);
}

void MockRedisServerBase::SendReplyData(const redis::ReplyData& reply_data) {
  SendReply(ReplyDataToRedisProto(reply_data));
}

int MockRedisServerBase::GetPort() const {
  return acceptor_.local_endpoint().port();
}

void MockRedisServerBase::Stop() {
  io_service_.stop();
  if (thread_.joinable()) thread_.join();
}

void MockRedisServerBase::SendReply(const std::string& reply) {
  LOG_DEBUG() << "reply: " << reply;
  // TODO: async?
  io::write(socket_, io::buffer(reply.c_str(), reply.size()));
}

std::string MockRedisServerBase::ReplyDataToRedisProto(
    const redis::ReplyData& reply_data) {
  switch (reply_data.GetType()) {
    case redis::ReplyData::Type::kString:
      return '$' + std::to_string(reply_data.GetString().size()) + kCrlf +
             reply_data.GetString() + kCrlf;
    case redis::ReplyData::Type::kArray: {
      std::string res =
          '*' + std::to_string(reply_data.GetArray().size()) + kCrlf;
      for (const auto& elem : reply_data.GetArray())
        res += ReplyDataToRedisProto(elem);
      return res;
    }
    case redis::ReplyData::Type::kInteger:
      return ':' + reply_data.ToDebugString() + kCrlf;
    case redis::ReplyData::Type::kNil:
      return "$-1" + kCrlf;
    case redis::ReplyData::Type::kStatus:
      return '+' + reply_data.ToDebugString() + kCrlf;
    case redis::ReplyData::Type::kError:
      return '-' + reply_data.ToDebugString() + kCrlf;
    default:
      throw std::runtime_error("can't send reply with data type=" +
                               reply_data.GetTypeString());
  }
}

void MockRedisServerBase::Accept() {
  acceptor_.async_accept(socket_, [this](auto&& item) {
    OnAccept(std::forward<decltype(item)>(item));
  });
}

void MockRedisServerBase::Work() {
  Accept();
  UEXPECT_NO_THROW(io_service_.run());
}

void MockRedisServerBase::OnAccept(boost::system::error_code ec) {
  LOG_DEBUG() << "accept(2): " << ec;
  OnConnected();
  DoRead();
}

void MockRedisServerBase::OnRead(boost::system::error_code ec, size_t count) {
  LOG_DEBUG() << "read " << ec << " count=" << count;
  if (ec) {
    LOG_DEBUG() << "read(2) error: " << ec;
    OnDisconnected();
    socket_.close();
    // allow reconnection after previous connection is closed
    Accept();
    return;
  }

  auto ret = redisReaderFeed(reader_.get(), data_.data(), count);
  if (ret != REDIS_OK)
    throw std::runtime_error("redisReaderFeed() returned error: " +
                             std::string(reader_->errstr));

  void* hiredis_reply = nullptr;
  while (redisReaderGetReply(reader_.get(), &hiredis_reply) == REDIS_OK &&
         hiredis_reply) {
    auto reply = std::make_shared<redis::Reply>(
        "", static_cast<redisReply*>(hiredis_reply), redis::ReplyStatus::kOk);
    LOG_DEBUG() << "command: " << reply->data.ToDebugString();

    OnCommand(reply);
    freeReplyObject(hiredis_reply);
    hiredis_reply = nullptr;
  }

  DoRead();
}

void MockRedisServerBase::DoRead() {
  socket_.async_read_some(io::buffer(data_),
                          [this](boost::system::error_code error_code,
                                 size_t count) { OnRead(error_code, count); });
}

MockRedisServer::~MockRedisServer() { Stop(); }

MockRedisServer::HandlerPtr MockRedisServer::RegisterHandlerWithConstReply(
    const std::string& command, redis::ReplyData reply_data) {
  return RegisterHandlerWithConstReply(command, {}, std::move(reply_data));
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterHandlerWithConstReply(
    const std::string& command, const std::vector<std::string>& args_prefix,
    redis::ReplyData reply_data) {
  auto handler = std::make_shared<Handler>();
  RegisterHandlerFunc(
      command, args_prefix,
      [this, handler, reply_data](const std::vector<std::string>&) {
        handler->AccountReply();
        SendReplyData(reply_data);
      });
  return handler;
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterStatusReplyHandler(
    const std::string& command, std::string reply) {
  return RegisterHandlerWithConstReply(
      command, redis::ReplyData::CreateStatus(std::move(reply)));
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterStatusReplyHandler(
    const std::string& command, const std::vector<std::string>& args_prefix,
    std::string reply) {
  return RegisterHandlerWithConstReply(
      command, args_prefix, redis::ReplyData::CreateStatus(std::move(reply)));
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterErrorReplyHandler(
    const std::string& command, std::string reply) {
  return RegisterHandlerWithConstReply(
      command, redis::ReplyData::CreateError(std::move(reply)));
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterErrorReplyHandler(
    const std::string& command, const std::vector<std::string>& args_prefix,
    std::string reply) {
  return RegisterHandlerWithConstReply(
      command, args_prefix, redis::ReplyData::CreateError(std::move(reply)));
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterNilReplyHandler(
    const std::string& command) {
  return RegisterHandlerWithConstReply(command, redis::ReplyData::CreateNil());
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterNilReplyHandler(
    const std::string& command, const std::vector<std::string>& args_prefix) {
  return RegisterHandlerWithConstReply(command, args_prefix,
                                       redis::ReplyData::CreateNil());
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterPingHandler() {
  ping_handler_ = RegisterStatusReplyHandler("PING", "PONG");
  return ping_handler_;
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterSentinelMastersHandler(
    const std::vector<MasterInfo>& masters) {
  std::vector<redis::ReplyData> reply_data;
  reply_data.reserve(masters.size());
  for (const auto& master : masters) {
    // TODO: add fields like 'role-reported', ... if needed
    reply_data.emplace_back(
        redis::ReplyData::Array{{"name"},
                                {master.name},
                                {"ip"},
                                {master.ip},
                                {"port"},
                                {std::to_string(master.port)},
                                {"flags"},
                                {boost::join(master.flags, ",")}});
  }
  return RegisterHandlerWithConstReply("SENTINEL", {"MASTERS"},
                                       std::move(reply_data));
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterSentinelSlavesHandler(
    const std::string& master_name, const std::vector<SlaveInfo>& slaves) {
  std::vector<redis::ReplyData> reply_data;
  reply_data.reserve(slaves.size());
  for (const auto& slave : slaves) {
    // TODO: add fields like 'role-reported', 'master-host', ... if needed
    reply_data.emplace_back(
        redis::ReplyData::Array{{"name"},
                                {slave.name},
                                {"ip"},
                                {slave.ip},
                                {"port"},
                                {std::to_string(slave.port)},
                                {"flags"},
                                {boost::join(slave.flags, ",")},
                                {"master-link-status"},
                                {slave.master_link_status}});
  }
  return RegisterHandlerWithConstReply("SENTINEL", {"SLAVES", master_name},
                                       std::move(reply_data));
}

void MockRedisServer::OnCommand(std::shared_ptr<redis::Reply> cmd) {
  ASSERT_TRUE(cmd->data.IsArray());
  auto array = cmd->data.GetArray();
  ASSERT_GT(array.size(), 0UL);
  ASSERT_TRUE(array[0].IsString());

  const auto& command = array[0].GetString();
  std::lock_guard<std::mutex> lock(mutex_);
  auto handler_it = handlers_.find(boost::algorithm::to_lower_copy(command));
  if (handler_it == handlers_.end()) {
    EXPECT_TRUE(false) << "Unknown command: " << command;
  } else {
    std::vector<std::string> args;
    for (size_t i = 1; i < array.size(); i++) {
      EXPECT_TRUE(array[i].IsString());
      args.push_back(array[i].GetString());
    }

    const auto& handler = GetHandlerFunc(handler_it->second, args);
    if (handler)
      handler(args);
    else
      HandlerHotFound(command, args);
  }
}

void MockRedisServer::RegisterHandlerFunc(
    std::string cmd, const std::vector<std::string>& args_prefix,
    HandlerFunc handler) {
  std::lock_guard<std::mutex> lock(mutex_);
  AddHandlerFunc(handlers_[boost::algorithm::to_lower_copy(cmd)], args_prefix,
                 std::move(handler));
}

void MockRedisServer::AddHandlerFunc(HandlerNode& root,
                                     const std::vector<std::string>& args,
                                     HandlerFunc handler) {
  auto* node = &root;

  for (const auto& arg : args) {
    node = &(node->next_arg[arg]);
  }
  node->handler = std::move(handler);
}

const MockRedisServer::HandlerFunc& MockRedisServer::GetHandlerFunc(
    const HandlerNode& node, const std::vector<std::string>& args,
    size_t arg_idx) {
  if (arg_idx < args.size()) {
    auto it = node.next_arg.find(args[arg_idx]);
    if (it != node.next_arg.end()) {
      const auto& handler = GetHandlerFunc(it->second, args, arg_idx + 1);
      if (handler) return handler;
    }
  }
  return node.handler;
}

MockRedisServer::HandlerPtr MockRedisServer::DoRegisterTimeoutHandler(
    const std::string& command, const std::vector<std::string>& args_prefix,
    const std::chrono::milliseconds& duration) {
  auto handler = std::make_shared<Handler>();
  RegisterHandlerFunc(
      command, args_prefix,
      [this, handler, duration](const std::vector<std::string>&) {
        std::this_thread::sleep_for(duration);
        handler->AccountReply();
        SendReplyData(redis::ReplyData::CreateStatus("OK"));
      });
  return handler;
}

size_t MockRedisServer::Handler::GetReplyCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return reply_count_;
}

void MockRedisServer::Handler::AccountReply() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    ++reply_count_;
  }
  cv_.notify_one();
}

MockRedisServer::CommonMasterSlaveInfo::CommonMasterSlaveInfo(
    std::string name, std::string ip, int port, std::vector<std::string> flags)
    : name(std::move(name)),
      ip(std::move(ip)),
      port(port),
      flags(std::move(flags)) {}

MockRedisServer::MasterInfo::MasterInfo(std::string name, std::string ip,
                                        int port,
                                        std::vector<std::string> flags)
    : CommonMasterSlaveInfo(std::move(name), std::move(ip), port, flags) {
  flags.emplace_back("master");
}

MockRedisServer::SlaveInfo::SlaveInfo(std::string name, std::string ip,
                                      int port, std::vector<std::string> flags,
                                      std::string master_link_status)
    : CommonMasterSlaveInfo(std::move(name), std::move(ip), port, flags),
      master_link_status(std::move(master_link_status)) {
  flags.emplace_back("slave");
}

USERVER_NAMESPACE_END
