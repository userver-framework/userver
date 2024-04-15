#pragma once

/// @file userver/formats/bson/exception.hpp
/// @brief BSON-specific exceptions

#include <bson/bson.h>

#include <userver/utils/traceful_exception.hpp>

#include <string>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {

/// Generic BSON-related exception
class BsonException : public utils::TracefulException {
 public:
  BsonException(std::string msg);

  std::string_view GetMessage() const noexcept { return msg_; }

 private:
  std::string msg_;
};

/// BSON parsing error
class ParseException : public BsonException {
 public:
  using BsonException::BsonException;
};

class ExceptionWithPath : public BsonException {
 public:
  explicit ExceptionWithPath(std::string_view msg, std::string_view path);

  std::string_view GetPath() const noexcept;
  std::string_view GetMessageWithoutPath() const noexcept;

 private:
  std::size_t path_size_;
};

/// BSON types mismatch error
class TypeMismatchException : public ExceptionWithPath {
 public:
  TypeMismatchException(bson_type_t actual, bson_type_t expected,
                        std::string_view path);
};

/// BSON array indexing error
class OutOfBoundsException : public ExceptionWithPath {
 public:
  OutOfBoundsException(size_t index, size_t size, std::string_view path);
};

/// BSON nonexisting member access error
class MemberMissingException : public ExceptionWithPath {
 public:
  explicit MemberMissingException(std::string_view path);
};

/// Conversion error
class ConversionException : public ExceptionWithPath {
 public:
  ConversionException(std::string_view msg, std::string_view path);
};

}  // namespace formats::bson

USERVER_NAMESPACE_END
