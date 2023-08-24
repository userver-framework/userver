#pragma once

/// @file userver/formats/bson/exception.hpp
/// @brief BSON-specific exceptions

#include <bson/bson.h>

#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {

/// Generic BSON-related exception
class BsonException : public utils::TracefulException {
 public:
  using utils::TracefulException::TracefulException;
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
                        std::string_view path);
};

/// BSON array indexing error
class OutOfBoundsException : public BsonException {
 public:
  OutOfBoundsException(size_t index, size_t size, std::string_view path);
};

/// BSON nonexisting member access error
class MemberMissingException : public BsonException {
 public:
  explicit MemberMissingException(std::string_view path);
};

/// Conversion error
class ConversionException : public BsonException {
 public:
  using BsonException::BsonException;
};

}  // namespace formats::bson

USERVER_NAMESPACE_END
