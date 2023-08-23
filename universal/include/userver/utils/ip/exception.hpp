#pragma once

/// @file userver/utils/ip/exception.hpp
/// @brief @copybrief utils::ip::AddressSystemError

#include <exception>
#include <string>
#include <system_error>

USERVER_NAMESPACE_BEGIN

// IP address and utilities
namespace utils::ip {

class AddressSystemError final : public std::exception {
 public:
  AddressSystemError(std::error_code code, std::string_view msg)
      : msg_(msg), code_(code) {}

  /// Operating system error code.
  const std::error_code& Code() const { return code_; }

  const char* what() const noexcept final { return msg_.c_str(); }

 private:
  std::string msg_;
  std::error_code code_;
};

}  // namespace utils::ip

USERVER_NAMESPACE_END
