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
  TypeMismatchException(const Json::Value& doc, Json::ValueType expected,
                        const std::string& path);
};

class OutOfBoundsException : public JsonException {
 public:
  OutOfBoundsException(size_t index, size_t size, const std::string& path);
};

}  // namespace json
}  // namespace formats
