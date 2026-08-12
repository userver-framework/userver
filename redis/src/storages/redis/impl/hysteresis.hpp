#pragma once
#include <atomic>
#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

/// Tracks node failure state based on consecutive failures/successes
class Hysteresis {
public:
    struct Config {
        size_t consecutive_failures{0};  // how many failures in a row trigger a failed state
        size_t consecutive_ok{0};        // how many successes in a row clear the failed state
    };

    // Default ctor – creates a “disabled” hysteresis (no state changes ever)
    Hysteresis();
    explicit Hysteresis(Config cfg);

    // Account a new result (true – failure, false – success)
    void AccountFail(bool fail);

    // Directly set the failed flag
    void SetFailed(bool failed) noexcept;

    // Query the current failed flag
    bool IsFailed() const noexcept;

    void SetConfig(Config config);

private:
    Config config_{};

    size_t consecutive_failures_{0};
    size_t consecutive_ok_{0};

    // The resulting state – true means the node is considered failed
    std::atomic_bool failed_{false};
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
