#include "mock_server_test.hpp"

#include <sstream>
#include <thread>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <boost/algorithm/string.hpp>

#include <userver/utest/assert_macros.hpp>
#include <userver/utils/numeric_cast.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kCrlf = "\r\n";

}  // namespace

MockRedisServerBase::MockRedisServerBase(int port) : acceptor_(io_service_) {
    acceptor_.open(io::ip::tcp::v4());
    const boost::asio::ip::tcp::acceptor::reuse_address option(true);
    acceptor_.set_option(option);
    acceptor_.bind(io::ip::tcp::endpoint(io::ip::tcp::v4(), port));
    acceptor_.listen();

    thread_ = std::thread(&MockRedisServerBase::Work, this);
}

MockRedisServerBase::~MockRedisServerBase() { Stop(); }

void MockRedisServerBase::SendReplyOk(ConnectionPtr connection, const std::string& reply) {
    SendReply(connection, '+' + reply + kCrlf);
}

void MockRedisServerBase::SendReplyError(ConnectionPtr connection, const std::string& reply) {
    SendReply(connection, '-' + reply + kCrlf);
}

void MockRedisServerBase::SendReplyData(ConnectionPtr connection, const storages::redis::ReplyData& reply_data) {
    SendReply(connection, ReplyDataToRedisProto(reply_data));
}

int MockRedisServerBase::GetPort() const { return acceptor_.local_endpoint().port(); }

void MockRedisServerBase::Stop() {
    io_service_.stop();
    if (thread_.joinable()) thread_.join();
}

void MockRedisServerBase::SendReply(ConnectionPtr connection, const std::string& reply) {
    LOG_DEBUG() << "reply: " << reply;
    // TODO: async?
    io::write(connection->socket, io::buffer(reply.c_str(), reply.size()));
}

std::string MockRedisServerBase::ReplyDataToRedisProto(const storages::redis::ReplyData& reply_data) {
    switch (reply_data.GetType()) {
        case storages::redis::ReplyData::Type::kString:
            return '$' + std::to_string(reply_data.GetString().size()) + kCrlf + reply_data.GetString() + kCrlf;
        case storages::redis::ReplyData::Type::kArray: {
            std::string res = '*' + std::to_string(reply_data.GetArray().size()) + kCrlf;
            for (const auto& elem : reply_data.GetArray()) res += ReplyDataToRedisProto(elem);
            return res;
        }
        case storages::redis::ReplyData::Type::kInteger:
            return ':' + reply_data.ToDebugString() + kCrlf;
        case storages::redis::ReplyData::Type::kNil:
            return "$-1" + kCrlf;
        case storages::redis::ReplyData::Type::kStatus:
            return '+' + reply_data.ToDebugString() + kCrlf;
        case storages::redis::ReplyData::Type::kError:
            return '-' + reply_data.ToDebugString() + kCrlf;
        default:
            throw std::runtime_error("can't send reply with data type=" + reply_data.GetTypeString());
    }
}

void MockRedisServerBase::Accept() {
    auto connection = std::make_shared<Connection>(io_service_);
    connection->reader =
        std::unique_ptr<redisReader, decltype(&redisReaderFree)>(redisReaderCreate(), &redisReaderFree);
    acceptor_.async_accept(connection->socket, [connection, this](auto item) {
        OnAccept(connection, std::move(item));
        Accept();
    });
}

void MockRedisServerBase::Work() {
    Accept();
    UEXPECT_NO_THROW(io_service_.run());
}

void MockRedisServerBase::OnAccept(ConnectionPtr connection, boost::system::error_code ec) {
    LOG_DEBUG() << "accept(2): " << ec;
    OnConnected(connection);
    DoRead(connection);
}

void MockRedisServerBase::OnRead(ConnectionPtr connection, boost::system::error_code ec, size_t count) {
    LOG_DEBUG() << "read " << ec << " count=" << count;
    if (ec) {
        LOG_DEBUG() << "read(2) error: " << ec;
        OnDisconnected(connection);
        connection->socket.close();
        return;
    }

    auto ret = redisReaderFeed(connection->reader.get(), connection->data.data(), count);
    if (ret != REDIS_OK)
        throw std::runtime_error("redisReaderFeed() returned error: " + std::string(connection->reader->errstr));

    void* hiredis_reply = nullptr;
    while (redisReaderGetReply(connection->reader.get(), &hiredis_reply) == REDIS_OK && hiredis_reply) {
        auto reply = std::make_shared<storages::redis::Reply>(
            "", static_cast<redisReply*>(hiredis_reply), storages::redis::ReplyStatus::kOk
        );
        LOG_DEBUG() << "command: " << reply->data.ToDebugString();

        OnCommand(connection, reply);
        freeReplyObject(hiredis_reply);
        hiredis_reply = nullptr;
    }

    DoRead(connection);
}

void MockRedisServerBase::DoRead(ConnectionPtr connection) {
    connection->socket.async_read_some(
        io::buffer(connection->data),
        [connection, this](boost::system::error_code error_code, size_t count) {
            OnRead(connection, error_code, count);
        }
    );
}

MockRedisServer::~MockRedisServer() { Stop(); }

MockRedisServer::HandlerPtr
MockRedisServer::RegisterHandlerWithConstReply(const std::string& command, storages::redis::ReplyData reply_data) {
    return RegisterHandlerWithConstReply(command, {}, std::move(reply_data));
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterHandlerWithConstReply(
    const std::string& command,
    const std::vector<std::string>& args_prefix,
    storages::redis::ReplyData reply_data
) {
    auto handler = std::make_shared<Handler>();
    RegisterHandlerFunc(
        command,
        args_prefix,
        [this, handler, reply_data](auto connect, const std::vector<std::string>&) {
            handler->AccountReply();
            SendReplyData(connect, reply_data);
        }
    );
    return handler;
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterStatusReplyHandler(const std::string& command, std::string reply) {
    return RegisterHandlerWithConstReply(command, storages::redis::ReplyData::CreateStatus(std::move(reply)));
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterStatusReplyHandler(
    const std::string& command,
    const std::vector<std::string>& args_prefix,
    std::string reply
) {
    return RegisterHandlerWithConstReply(
        command, args_prefix, storages::redis::ReplyData::CreateStatus(std::move(reply))
    );
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterErrorReplyHandler(const std::string& command, std::string reply) {
    return RegisterHandlerWithConstReply(command, storages::redis::ReplyData::CreateError(std::move(reply)));
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterErrorReplyHandler(
    const std::string& command,
    const std::vector<std::string>& args_prefix,
    std::string reply
) {
    return RegisterHandlerWithConstReply(
        command, args_prefix, storages::redis::ReplyData::CreateError(std::move(reply))
    );
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterNilReplyHandler(const std::string& command) {
    return RegisterHandlerWithConstReply(command, storages::redis::ReplyData::CreateNil());
}

MockRedisServer::HandlerPtr
MockRedisServer::RegisterNilReplyHandler(const std::string& command, const std::vector<std::string>& args_prefix) {
    return RegisterHandlerWithConstReply(command, args_prefix, storages::redis::ReplyData::CreateNil());
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterPingHandler() {
    ping_handler_ = RegisterStatusReplyHandler("PING", "PONG");
    return ping_handler_;
}

MockRedisServer::HandlerPtr MockRedisServer::RegisterSentinelMastersHandler(const std::vector<MasterInfo>& masters) {
    std::vector<storages::redis::ReplyData> reply_data;
    reply_data.reserve(masters.size());
    for (const auto& master : masters) {
        // TODO: add fields like 'role-reported', ... if needed
        reply_data.emplace_back(storages::redis::ReplyData::Array{
            {"name"},
            {master.name},
            {"ip"},
            {master.ip},
            {"port"},
            {std::to_string(master.port)},
            {"flags"},
            {fmt::to_string(fmt::join(master.flags, ","))}});
    }
    return RegisterHandlerWithConstReply("SENTINEL", {"MASTERS"}, std::move(reply_data));
}

MockRedisServer::HandlerPtr
MockRedisServer::RegisterSentinelSlavesHandler(const std::string& master_name, const std::vector<SlaveInfo>& slaves) {
    std::vector<storages::redis::ReplyData> reply_data;
    reply_data.reserve(slaves.size());
    for (const auto& slave : slaves) {
        // TODO: add fields like 'role-reported', 'master-host', ... if needed
        reply_data.emplace_back(storages::redis::ReplyData::Array{
            {"name"},
            {slave.name},
            {"ip"},
            {slave.ip},
            {"port"},
            {std::to_string(slave.port)},
            {"flags"},
            {fmt::to_string(fmt::join(slave.flags, ","))},
            {"master-link-status"},
            {slave.master_link_status}});
    }
    return RegisterHandlerWithConstReply("SENTINEL", {"SLAVES", master_name}, std::move(reply_data));
}

MockRedisServer::HandlerPtr
MockRedisServer::RegisterClusterNodes(const std::vector<MasterInfo>& masters, const std::vector<SlaveInfo>& slaves) {
    constexpr unsigned short kCommunicationPortIgnored = 16379;
    std::string nodes_reply_data;
    for (const auto& master : masters) {
        nodes_reply_data +=
            fmt::format("{} {}:{}@{} master\n", master.name, master.ip, master.port, kCommunicationPortIgnored);
    }
    for (const auto& slave : slaves) {
        nodes_reply_data +=
            fmt::format("{} {}:{}@{} slave\n", slave.name, slave.ip, slave.port, kCommunicationPortIgnored);
    }
    auto handler = RegisterHandlerWithConstReply("CLUSTER", {"NODES"}, {std::move(nodes_reply_data)});

    std::vector<storages::redis::ReplyData> slots_reply_data;
    slots_reply_data.reserve(masters.size());
    constexpr std::size_t kSlotsCount = 16384;
    const std::size_t slots_chunk_size = kSlotsCount / masters.size();
    UASSERT_MSG(
        masters.size() == slaves.size(),
        "At the moment MockServer supports clusters only with count of masters equal to slaves count"
    );
    for (std::size_t i = 0; i < masters.size(); ++i) {
        slots_reply_data.emplace_back(storages::redis::ReplyData::Array{
            utils::numeric_cast<int>(i * slots_chunk_size),
            utils::numeric_cast<int>(i + 1 == masters.size() ? kSlotsCount - 1 : (i + 1) * slots_chunk_size),
            storages::redis::ReplyData::Array{masters[i].ip, masters[i].port},
            storages::redis::ReplyData::Array{slaves[i].ip, slaves[i].port}});
    }
    RegisterHandlerWithConstReply("CLUSTER", {"SLOTS"}, {std::move(slots_reply_data)});
    return handler;
}

void MockRedisServer::OnCommand(ConnectionPtr connection, std::shared_ptr<storages::redis::Reply> cmd) {
    ASSERT_TRUE(cmd->data.IsArray());
    auto array = cmd->data.GetArray();
    ASSERT_GT(array.size(), 0UL);
    ASSERT_TRUE(array[0].IsString());

    const auto& command = array[0].GetString();
    const std::lock_guard lock{mutex_};
    std::vector<std::string> args;
    args.reserve(array.size());
    for (size_t i = 1; i < array.size(); i++) {
        EXPECT_TRUE(array[i].IsString());
        args.push_back(array[i].GetString());
    }

    auto handler_it = handlers_.find(boost::algorithm::to_lower_copy(command));
    if (handler_it == handlers_.end()) {
        EXPECT_TRUE(false) << fmt::format("Unknown for {} command {} {}", description_, command, fmt::join(args, " "));
        return;
    }

    const auto& handler = GetHandlerFunc(handler_it->second, args);
    ASSERT_TRUE(handler
    ) << fmt::format("Handler not found for {} command {} {}", description_, command, fmt::join(args, " "));
    handler(connection, args);
}

void MockRedisServer::RegisterHandlerFunc(
    std::string cmd,
    const std::vector<std::string>& args_prefix,
    HandlerFunc handler
) {
    const std::lock_guard<std::mutex> lock(mutex_);
    AddHandlerFunc(handlers_[boost::algorithm::to_lower_copy(cmd)], args_prefix, std::move(handler));
}

void MockRedisServer::AddHandlerFunc(HandlerNode& root, const std::vector<std::string>& args, HandlerFunc handler) {
    auto* node = &root;

    for (const auto& arg : args) {
        node = &(node->next_arg[arg]);
    }
    node->handler = std::move(handler);
}

const MockRedisServer::HandlerFunc&
MockRedisServer::GetHandlerFunc(const HandlerNode& node, const std::vector<std::string>& args, size_t arg_idx) {
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
    const std::string& command,
    const std::vector<std::string>& args_prefix,
    const std::chrono::milliseconds& duration
) {
    auto handler = std::make_shared<Handler>();
    RegisterHandlerFunc(
        command,
        args_prefix,
        [this, handler, duration](auto connection, const std::vector<std::string>&) {
            std::this_thread::sleep_for(duration);
            handler->AccountReply();
            SendReplyData(connection, storages::redis::ReplyData::CreateStatus("OK"));
        }
    );
    return handler;
}

size_t MockRedisServer::Handler::GetReplyCount() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return reply_count_;
}

void MockRedisServer::Handler::AccountReply() {
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        ++reply_count_;
    }
    cv_.notify_one();
}

MockRedisServer::CommonMasterSlaveInfo::CommonMasterSlaveInfo(
    std::string name,
    std::string ip,
    int port,
    std::vector<std::string> flags
)
    : name(std::move(name)), ip(std::move(ip)), port(port), flags(std::move(flags)) {}

MockRedisServer::MasterInfo::MasterInfo(std::string name, std::string ip, int port, std::vector<std::string> flags)
    : CommonMasterSlaveInfo(std::move(name), std::move(ip), port, flags) {
    flags.emplace_back("master");
}

MockRedisServer::SlaveInfo::SlaveInfo(
    std::string name,
    std::string ip,
    int port,
    std::vector<std::string> flags,
    std::string master_link_status
)
    : CommonMasterSlaveInfo(std::move(name), std::move(ip), port, flags),
      master_link_status(std::move(master_link_status)) {
    flags.emplace_back("slave");
}

USERVER_NAMESPACE_END
