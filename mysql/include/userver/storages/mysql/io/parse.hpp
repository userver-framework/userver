#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::io {

template <typename T>
class BaseParser {
 public:
  BaseParser(T& field) : field_{field} {}

  void operator()(std::string& source) { Parse(source); }

 protected:
  virtual void Parse(std::string& source) = 0;
  T& field_;
};

template <typename T>
class FieldParser final {
  FieldParser(T&) {
    static_assert(!sizeof(T), "Parser for the type is not implemented");
  }
};

template <>
class FieldParser<std::string> final : public BaseParser<std::string> {
  using BaseParser::BaseParser;

  void Parse(std::string& source) override;
};

template <>
class FieldParser<int> final : public BaseParser<int> {
  using BaseParser::BaseParser;

  void Parse(std::string& source) override;
};

template <typename T>
auto GetParser(T& field) {
  return FieldParser<T>{field};
}

}  // namespace storages::mysql::io

USERVER_NAMESPACE_END
