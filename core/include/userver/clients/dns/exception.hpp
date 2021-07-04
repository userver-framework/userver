#pragma once

/// @file userver/clients/dns/exception.hpp
/// @brief DNS client exceptions

#include <stdexcept>

namespace clients::dns {

/// Generic resolver error
class ResolverException : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// Unsupported domain error
class UnsupportedDomainException : public ResolverException {
 public:
  using ResolverException::ResolverException;
};

/// Host resolution error
class NotResolvedException : public ResolverException {
 public:
  using ResolverException::ResolverException;
};

}  // namespace clients::dns
