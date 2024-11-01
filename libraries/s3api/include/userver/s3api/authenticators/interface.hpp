#pragma once

/// @file userver/s3api/authenticators/interface.hpp
/// @brief Interface for autheticators - classes that sign the request with auth data

#include <memory>
#include <string>
#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace s3api {

struct Request;

namespace authenticators {

// This is base class for all authenticators
struct Authenticator {
    virtual std::unordered_map<std::string, std::string> Auth(const Request& request) const = 0;
    virtual std::unordered_map<std::string, std::string> Sign(const Request& request, time_t expires) const = 0;
    virtual ~Authenticator() = default;
};

using AuthenticatorPtr = std::shared_ptr<Authenticator>;

}  // namespace authenticators
}  // namespace s3api

USERVER_NAMESPACE_END
