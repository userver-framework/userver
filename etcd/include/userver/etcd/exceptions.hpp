#pragma once

/// @file userver/etcd/exceptions.hpp
/// @brief Exceptions thrown by etcd client

#include <string>

#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace etcd {

/// @brief Base class for all etcd client exceptions
class EtcdError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// @brief Error during a request to etcd
class EtcdRequestError : public EtcdError {
public:
    using EtcdError::EtcdError;
};

/// @brief Error during parsing of etcd watch response
class EtcdWatchResponseParseError : public EtcdError {
public:
    using EtcdError::EtcdError;
};

}  // namespace etcd

USERVER_NAMESPACE_END
