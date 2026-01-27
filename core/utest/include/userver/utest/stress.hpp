#pragma once

/// @file userver/utest/stress.hpp
/// @brief @copybrief utest::StressLoop

#include <chrono>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

/// @brief Helper for stress tests for watching for
/// "OK, we have been running for enough time, let's stop" event.
///
/// Example:
/// @code
/// for (auto _: utest::StressLoop()) {
///     ...
/// }
/// @endcode
class StressLoop {
public:
    struct Iterator {
        StressLoop& loop;

        struct Value {
            ~Value() {
                (void)0;  // force non-trivial destructor
            }
        };

        void operator++() {}
        Value operator*() { return {}; }
        bool operator==(const Iterator&) const { return loop.ShouldStop(); }
        bool operator!=(const Iterator&) const { return !loop.ShouldStop(); }
    };

    Iterator begin() { return {*this}; }

    Iterator end() { return {*this}; }

    bool ShouldStop() const { return deadline_.IsReached(); }

private:
    static constexpr std::chrono::seconds kMaxStressTestWaitTime{30};

    engine::Deadline deadline_{engine::Deadline::FromDuration(kMaxStressTestWaitTime)};
};

}  // namespace utest

USERVER_NAMESPACE_END
