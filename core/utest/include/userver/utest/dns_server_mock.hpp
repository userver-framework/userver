#pragma once

#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/task/task.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io::tests {
struct UdpListener;
}  // namespace engine::io::tests

namespace utest {

class DnsServerMock final {
public:
    enum class RecordType {
        kInvalid = 0,
        kA = 1,
        kAAAA = 28,
        kCname = 5,
    };

    struct DnsQuery {
        RecordType type{RecordType::kInvalid};
        std::string name;
    };

    struct DnsAnswer {
        using AnswerData = std::variant<std::monostate, engine::io::Sockaddr, std::string>;

        RecordType type{RecordType::kInvalid};
        AnswerData data;
        int ttl{0};
    };

    using DnsAnswerVector = std::vector<DnsAnswer>;

    struct NoAnswer : std::exception {};

    // throwing an exception will cause SERVFAIL
    using DnsHandler = std::function<DnsAnswerVector(const DnsQuery&)>;

    explicit DnsServerMock(DnsHandler);

    std::string GetServerAddress() const;

private:
    void ProcessRequests();

    std::shared_ptr<engine::io::tests::UdpListener> listener_;
    DnsHandler handler_;
    engine::Task receiver_task_;
};

}  // namespace utest

USERVER_NAMESPACE_END
