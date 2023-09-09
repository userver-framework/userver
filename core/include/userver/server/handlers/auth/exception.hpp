#pragma once

/// @file userver/server/handlers/auth/exception.hpp
/// @brief Exception classes for server::handlers::auth::DigestParser

#include <stdexcept>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

class Exception : public std::exception {
 public:
  explicit Exception(std::string msg) : msg_(std::move(msg)) {}

  const char* what() const noexcept final { return msg_.c_str(); }

 private:
  std::string msg_;
};

class ParseException : public Exception {
 public:
  using Exception::Exception;
};

class DuplicateDirectiveException : public Exception {
 public:
  using Exception::Exception;
};

class MissingDirectivesException : public Exception {
 public:
  MissingDirectivesException(std::vector<std::string>&& missing_directives);

  const std::vector<std::string>& GetMissingDirectives() const noexcept;

 private:
  std::vector<std::string> missing_directives_;
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
