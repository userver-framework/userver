#include <userver/storages/redis/command_options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

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
