#pragma once

#include <chrono>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class RetryBackoff final {
public:
    RetryBackoff();

    std::chrono::milliseconds NextAttemptDelay();

    void Reset();

private:
    bool initial_{};
    std::int64_t current_backoff_ms_{};
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
