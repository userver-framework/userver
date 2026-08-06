#include "hysteresis.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

Hysteresis::Hysteresis() = default;

Hysteresis::Hysteresis(Config cfg)
    : config_(cfg)
{}

void Hysteresis::AccountFail(bool fail) {
    if (fail) {
        // a failure – increase failure counter, reset success counter
        ++consecutive_failures_;
        consecutive_ok_ = 0;
        if (!failed_.load() && config_.consecutive_failures != 0 &&
            consecutive_failures_ >= config_.consecutive_failures)
        {
            SetFailed(true);
        }
    } else {
        // a success – increase success counter, reset failure counter
        ++consecutive_ok_;
        consecutive_failures_ = 0;
        if (failed_.load() && config_.consecutive_ok != 0 && consecutive_ok_ >= config_.consecutive_ok) {
            SetFailed(false);
        }
    }
}

void Hysteresis::SetFailed(bool failed) noexcept { failed_ = failed; }

bool Hysteresis::IsFailed() const noexcept { return failed_.load(); }

void Hysteresis::SetConfig(Config config) {
    config_ = config;
    consecutive_failures_ = 0;
    consecutive_ok_ = 0;
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
