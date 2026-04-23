#pragma once

#include <chrono>

#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

class ScyllaException : public utils::TracefulException {
public:
    ScyllaException();

    explicit ScyllaException(std::string_view what);
};

class InvalidConfigException : public ScyllaException {
    using ScyllaException::ScyllaException;
};

class NetworkException : public ScyllaException {
    using ScyllaException::ScyllaException;
};

class QueryException : public ScyllaException {
    using ScyllaException::ScyllaException;
};

class InvalidQueryArgumentException : public QueryException {
    using QueryException::QueryException;
};

class AuthenticationException : public ScyllaException {
    using ScyllaException::ScyllaException;
};

class ClusterUnavailableException : public ScyllaException {
    using ScyllaException::ScyllaException;
};

class ServerException : public QueryException {
public:
    ServerException(int code, std::string_view what);

    int Code() const noexcept { return code_; }

private:
    int code_;
};

class TimeoutException : public ServerException {
public:
    TimeoutException(int code, std::string_view what, std::chrono::milliseconds timeout);

    std::chrono::milliseconds Timeout() const noexcept { return timeout_; }

private:
    std::chrono::milliseconds timeout_;
};

class PoolOverloadException : public ScyllaException {
    using ScyllaException::ScyllaException;
};

class CancelledException : public ScyllaException {
public:
    struct ByDeadlinePropagation final {};

    using ScyllaException::ScyllaException;
    explicit CancelledException(ByDeadlinePropagation);

    bool IsByDeadlinePropagation() const noexcept { return by_deadline_propagation_; }

private:
    bool by_deadline_propagation_{false};
};

}  // namespace storages::scylla

USERVER_NAMESPACE_END
