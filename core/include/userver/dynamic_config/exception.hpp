#pragma once

#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

class Error : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class ConfigParseError : public Error {
 public:
  using Error::Error;
};

}  // namespace dynamic_config

USERVER_NAMESPACE_END
