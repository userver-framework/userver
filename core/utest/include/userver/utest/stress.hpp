#pragma once

/// @file userver/utest/stress.hpp
/// @brief @copybrief utest::StressLoop

#include <chrono>

#include <userver/engine/deadline.hpp>

#include <userver/compiler/impl/asan.hpp>
#include <userver/compiler/impl/tsan.hpp>

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
#if USERVER_IMPL_HAS_TSAN || USERVER_IMPL_HAS_ASAN
    // Too many iterations with sanitizers may consume too much virtual memory and crach the application
    static constexpr std::chrono::milliseconds kMaxStressTestWaitTime{300};
#else
    static constexpr std::chrono::seconds kMaxStressTestWaitTime{30};
#endif

    engine::Deadline deadline_{engine::Deadline::FromDuration(kMaxStressTestWaitTime)};
};

}  // namespace utest

USERVER_NAMESPACE_END
