#pragma once

/// @file userver/formats/json/exception.hpp
/// @brief Exception classes for JSON module

#include <iosfwd>
#include <stdexcept>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace formats {
namespace json {

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

class BadStreamException : public Exception {
 public:
  explicit BadStreamException(const std::istream& is);
  explicit BadStreamException(const std::ostream& os);
};

class TypeMismatchException : public Exception {
 public:
  TypeMismatchException(int actual, int expected, const std::string& path);
};

class OutOfBoundsException : public Exception {
 public:
  OutOfBoundsException(size_t index, size_t size, const std::string& path);
};

class MemberMissingException : public Exception {
 public:
  explicit MemberMissingException(const std::string& path);
};

/// Conversion error
class ConversionException : public Exception {
 public:
  using Exception::Exception;
};

}  // namespace json
}  // namespace formats

USERVER_NAMESPACE_END
