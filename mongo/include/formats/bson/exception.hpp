#pragma once

/// @file formats/bson/exception.hpp
/// @brief BSON-specific exceptions

#include <stdexcept>
#include <string>

#include <bson/bson.h>

namespace formats::bson {

/// Generic BSON-related exception
class BsonException : public std::exception {
 public:
  explicit BsonException(std::string msg) : msg_(std::move(msg)) {}

  virtual const char* what() const noexcept override { return msg_.c_str(); }

 private:
  std::string msg_;
};

/// BSON parsing error
class ParseException : public BsonException {
 public:
  using BsonException::BsonException;
};

/// BSON types mismatch error
class TypeMismatchException : public BsonException {
 public:
  TypeMismatchException(bson_type_t actual, bson_type_t expected,
                        const std::string& path);
};

/// BSON array indexing error
class OutOfBoundsException : public BsonException {
 public:
  OutOfBoundsException(size_t index, size_t size, const std::string& path);
};

/// BSON nonexisting member access error
class MemberMissingException : public BsonException {
 public:
  explicit MemberMissingException(const std::string& path);
};

}  // namespace formats::bson
