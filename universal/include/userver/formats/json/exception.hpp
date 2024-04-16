#pragma once

/// @file userver/formats/json/exception.hpp
/// @brief Exception classes for JSON module
/// @ingroup userver_universal

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

  std::string_view GetMessage() const noexcept { return msg_; }

 private:
  std::string msg_;
};

class ParseException : public Exception {
 public:
  using Exception::Exception;
};

class ExceptionWithPath : public Exception {
 public:
  explicit ExceptionWithPath(std::string_view msg, std::string_view path);

  std::string_view GetPath() const noexcept;
  std::string_view GetMessageWithoutPath() const noexcept;

 private:
  std::size_t path_size_;
};

class BadStreamException : public Exception {
 public:
  explicit BadStreamException(const std::istream& is);
  explicit BadStreamException(const std::ostream& os);
};

class TypeMismatchException : public ExceptionWithPath {
 public:
  TypeMismatchException(int actual, int expected, std::string_view path);
  std::string_view GetActual() const;
  std::string_view GetExpected() const;

 private:
  int actual_;
  int expected_;
};

class OutOfBoundsException : public ExceptionWithPath {
 public:
  OutOfBoundsException(size_t index, size_t size, std::string_view path);
};

class MemberMissingException : public ExceptionWithPath {
 public:
  explicit MemberMissingException(std::string_view path);
};

/// Conversion error
class ConversionException : public ExceptionWithPath {
 public:
  ConversionException(std::string_view msg, std::string_view path);
};

class UnknownDiscriminatorException : public ExceptionWithPath {
 public:
  UnknownDiscriminatorException(std::string_view path,
                                std::string_view discriminator_field);
};

}  // namespace formats::json

USERVER_NAMESPACE_END
