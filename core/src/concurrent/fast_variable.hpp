#pragma once

#include <userver/concurrent/variable.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

template <typename Data, typename Mutex = engine::Mutex>
class FastVariable final {
public:
    template <typename... Arg>
    FastVariable(Arg&&... arg)
        : var_(std::forward<Arg>(arg)...)
    {}

    void DisableWrites() { writes_disabled_ = true; }

    bool AreWritesDisabled() const { return writes_disabled_; }

    LockedPtr<std::unique_lock<Mutex>, Data> UniqueLock() {
        UINVARIANT(!writes_disabled_, "Writes are disabled");
        return var_.UniqueLock();
    }

    // Slow bore DisableWrites(), very fast afterwards.
    LockedPtr<std::unique_lock<Mutex>, const Data> SharedLock() const {
        if (writes_disabled_) {
            return {{}, var_.GetDataUnsafe()};
        } else {
            return var_.UniqueLock();
        }
    }

private:
    std::atomic<bool> writes_disabled_{false};
    Variable<Data, Mutex> var_;
};

}  // namespace concurrent

USERVER_NAMESPACE_END
