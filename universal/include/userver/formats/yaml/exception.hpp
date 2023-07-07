#pragma once

/// @file userver/formats/yaml/exception.hpp
/// @brief Exception classes for YAML module

#include <iosfwd>
#include <stdexcept>
#include <string>

#include <userver/formats/yaml/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::yaml {

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
  TypeMismatchException(Type actual, Type expected, std::string_view path);
  TypeMismatchException(int actual, int expected, std::string_view path);
  TypeMismatchException(const YAML::Node& value, std::string_view expected_type,
                        std::string_view path);
};

class OutOfBoundsException : public Exception {
 public:
  OutOfBoundsException(size_t index, size_t size, std::string_view path);
};

class MemberMissingException : public Exception {
 public:
  explicit MemberMissingException(std::string_view path);
};

class PathPrefixException : public Exception {
 public:
  explicit PathPrefixException(std::string_view old_path,
                               std::string_view prefix);
};

}  // namespace formats::yaml

USERVER_NAMESPACE_END
