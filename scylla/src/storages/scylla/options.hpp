#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::options {

class Consistency {
public:
    enum Level {
        kOne,
        kTwo,
        kThree,
        kQuorum,
        kAll,
        kLocalQuorum,
        kEachQuorum,
        kLocalOne,
        kAny,
    };

    explicit Consistency(Level level)
        : level_(level)
    {}

    Level GetLevel() const { return level_; }

private:
    Level level_;
};

class SerialConsistency {
public:
    enum Level {

        kSerial,

        kLocalSerial,
    };

    explicit SerialConsistency(Level level)
        : level_(level)
    {}

    Level GetLevel() const { return level_; }

private:
    Level level_;
};

class Timeout {
public:
    explicit Timeout(std::chrono::milliseconds value)
        : value_(value)
    {}

    const std::chrono::milliseconds& Value() const { return value_; }

private:
    std::chrono::milliseconds value_;
};

class Timestamp {
public:
    explicit Timestamp(std::chrono::microseconds value)
        : value_(value)
    {}

    explicit Timestamp(std::chrono::system_clock::time_point tp)
        : value_(std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch()))
    {}

    std::chrono::microseconds Value() const { return value_; }
    int64_t Microseconds() const { return value_.count(); }

private:
    std::chrono::microseconds value_;
};

class Ttl {
public:
    explicit Ttl(std::chrono::seconds value)
        : value_(value)
    {}

    std::chrono::seconds Value() const { return value_; }
    int32_t Seconds() const { return static_cast<int32_t>(value_.count()); }

private:
    std::chrono::seconds value_;
};

class Tracing {
public:
    explicit Tracing(bool enabled = true)
        : enabled_(enabled)
    {}

    bool IsEnabled() const { return enabled_; }

private:
    bool enabled_;
};

class PageSize {
public:
    explicit PageSize(int32_t value)
        : value_(value)
    {}

    int32_t Value() const { return value_; }

private:
    int32_t value_;
};

class PagingState {
public:
    explicit PagingState(std::string token)
        : token_(std::move(token))
    {}

    const std::string& Token() const { return token_; }

private:
    std::string token_;
};

class RoutingKey {
public:
    explicit RoutingKey(std::string value)
        : value_(std::move(value))
    {}

    const std::string& Value() const { return value_; }

private:
    std::string value_;
};

class RetryPolicy {
public:
    enum Policy {

        kDefault,

        kFallthrough,
    };

    explicit RetryPolicy(Policy policy)
        : policy_(policy)
    {}

    Policy GetPolicy() const { return policy_; }

private:
    Policy policy_;
};

class SpeculativeExecution {
public:
    SpeculativeExecution(int32_t max_attempts, std::chrono::milliseconds delay)
        : max_attempts_(max_attempts),
          delay_(delay)
    {}

    int32_t MaxAttempts() const { return max_attempts_; }
    std::chrono::milliseconds Delay() const { return delay_; }

private:
    int32_t max_attempts_;
    std::chrono::milliseconds delay_;
};

class IfNotExists {};

class IfExists {};

class AllowFiltering {};

class Unlogged {};

class SuppressServerExceptions {};

class Comment {
public:
    explicit Comment(std::string value)
        : value_(std::move(value))
    {}

    const std::string& Value() const { return value_; }

private:
    std::string value_;
};

}  // namespace storages::scylla::options

USERVER_NAMESPACE_END
