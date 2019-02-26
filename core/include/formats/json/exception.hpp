#pragma once

#include <iosfwd>
#include <stdexcept>

#include <json/value.h>

namespace formats {
namespace json {

class JsonException : public std::exception {
 public:
  explicit JsonException(std::string msg) : msg_(std::move(msg)) {}

  virtual const char* what() const noexcept override { return msg_.c_str(); }

 private:
  std::string msg_;
};

class ParseException : public JsonException {
 public:
  using JsonException::JsonException;
};

class BadStreamException : public JsonException {
 public:
  explicit BadStreamException(const std::istream& is);
  explicit BadStreamException(const std::ostream& os);
};

class TypeMismatchException : public JsonException {
 public:
  TypeMismatchException(Json::ValueType actual, Json::ValueType expected,
                        const std::string& path);
};

class OutOfBoundsException : public JsonException {
 public:
  OutOfBoundsException(size_t index, size_t size, const std::string& path);
};

class IntegralOverflowException : public JsonException {
 public:
  IntegralOverflowException(int64_t min, int64_t value, int64_t max,
                            const std::string& path);

  IntegralOverflowException(uint64_t min, uint64_t value, uint64_t max,
                            const std::string& path);
};

class MemberMissingException : public JsonException {
 public:
  explicit MemberMissingException(const std::string& path);
};

}  // namespace json
}  // namespace formats
