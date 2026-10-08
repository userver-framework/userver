#include <userver/storages/redis/command_options.hpp>

#include <utility>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

ExecCondition::ExecCondition(Type type, std::string key, std::string value)
    : type_(type),
      key_(std::move(key)),
      value_(std::move(value))
{}

ExecCondition ExecCondition::IfEq(std::string key, std::string value) {
    return ExecCondition{Type::kIfEq, std::move(key), std::move(value)};
}

ExecCondition ExecCondition::IfNe(std::string key, std::string value) {
    return ExecCondition{Type::kIfNe, std::move(key), std::move(value)};
}

ExecCondition ExecCondition::Nx(std::string key) { return ExecCondition{Type::kNx, std::move(key), {}}; }

ExecCondition ExecCondition::Xx(std::string key) { return ExecCondition{Type::kXx, std::move(key), {}}; }

MsetexOptions MsetexOptions::NoTtl() {
    return MsetexOptions{Exist::kSetAlways, TtlAction::kNone, std::chrono::milliseconds{0}};
}

MsetexOptions MsetexOptions::KeepTtl() {
    return MsetexOptions{Exist::kSetAlways, TtlAction::kKeepTtl, std::chrono::milliseconds{0}};
}

MsetexOptions MsetexOptions::Expire(std::chrono::milliseconds ttl) {
    return MsetexOptions{Exist::kSetAlways, TtlAction::kSetMilliseconds, ttl};
}

MsetexOptions MsetexOptions::ExpireAt(std::chrono::system_clock::time_point deadline) {
    return MsetexOptions{
        Exist::kSetAlways,
        TtlAction::kSetAtMilliseconds,
        std::chrono::duration_cast<std::chrono::milliseconds>(deadline.time_since_epoch())
    };
}

HgetexOptions HgetexOptions::Keep() { return HgetexOptions{TtlAction::kKeep, std::chrono::milliseconds{0}}; }

HgetexOptions HgetexOptions::Persist() { return HgetexOptions{TtlAction::kPersist, std::chrono::milliseconds{0}}; }

HgetexOptions HgetexOptions::Expire(std::chrono::milliseconds ttl) {
    return HgetexOptions{TtlAction::kSetMilliseconds, ttl};
}

HgetexOptions HgetexOptions::ExpireAt(std::chrono::system_clock::time_point deadline) {
    return HgetexOptions{
        TtlAction::kSetAtMilliseconds,
        std::chrono::duration_cast<std::chrono::milliseconds>(deadline.time_since_epoch())
    };
}

HsetexOptions HsetexOptions::NoTtl() {
    return HsetexOptions{Exist::kSetAlways, TtlAction::kNone, std::chrono::milliseconds{0}};
}

HsetexOptions HsetexOptions::KeepTtl() {
    return HsetexOptions{Exist::kSetAlways, TtlAction::kKeepTtl, std::chrono::milliseconds{0}};
}

HsetexOptions HsetexOptions::Expire(std::chrono::milliseconds ttl) {
    return HsetexOptions{Exist::kSetAlways, TtlAction::kSetMilliseconds, ttl};
}

HsetexOptions HsetexOptions::ExpireAt(std::chrono::system_clock::time_point deadline) {
    return HsetexOptions{
        Exist::kSetAlways,
        TtlAction::kSetAtMilliseconds,
        std::chrono::duration_cast<std::chrono::milliseconds>(deadline.time_since_epoch())
    };
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
