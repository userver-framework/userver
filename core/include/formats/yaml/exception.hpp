#pragma once

#include <iosfwd>
#include <stdexcept>

#include <formats/yaml/types.hpp>

namespace formats {
namespace yaml {

class YamlException : public std::exception {
 public:
  explicit YamlException(std::string msg) : msg_(std::move(msg)) {}

  const char* what() const noexcept final { return msg_.c_str(); }

 private:
  std::string msg_;
};

class ParseException : public YamlException {
 public:
  using YamlException::YamlException;
};

class BadStreamException : public YamlException {
 public:
  explicit BadStreamException(const std::istream& is);
  explicit BadStreamException(const std::ostream& os);
};

class TypeMismatchException : public YamlException {
 public:
  TypeMismatchException(Type actual, Type expected, const std::string& path);
  TypeMismatchException(const YAML::Node& value,
                        const std::string& type_expected,
                        const std::string& path);
};

class OutOfBoundsException : public YamlException {
 public:
  OutOfBoundsException(size_t index, size_t size, const std::string& path);
};

class IntegralOverflowException : public YamlException {
 public:
  IntegralOverflowException(int64_t min, int64_t value, int64_t max,
                            const std::string& path);

  IntegralOverflowException(uint64_t min, uint64_t value, uint64_t max,
                            const std::string& path);
};

class MemberMissingException : public YamlException {
 public:
  explicit MemberMissingException(const std::string& path);
};

}  // namespace yaml
}  // namespace formats
