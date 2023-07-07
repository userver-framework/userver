#pragma once

/// @file userver/formats/json/exception.hpp
/// @brief Exception classes for JSON module

#include <iosfwd>
#include <stdexcept>
#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

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
  std::string_view GetActual() const;
  std::string_view GetExpected() const;
  const std::string& GetPath() const noexcept;

 private:
  int actual_;
  int expected_;
  std::string path_;
};

class OutOfBoundsException : public Exception {
 public:
  OutOfBoundsException(size_t index, size_t size, const std::string& path);
  const std::string& GetPath() const noexcept;

 private:
  std::string path_;
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

}  // namespace formats::json

USERVER_NAMESPACE_END
