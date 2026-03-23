#pragma once

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

}  // namespace storages::scylla

USERVER_NAMESPACE_END