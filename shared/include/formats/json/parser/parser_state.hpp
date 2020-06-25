#pragma once

#include <string>

#include <utils/fast_pimpl.hpp>

namespace formats::json::parser {

class BaseParser;
class ParserHandler;

class ParserState final {
 public:
  ParserState();
  ParserState(const ParserState&) = delete;
  ParserState(ParserState&&) = delete;
  ~ParserState();

  ParserState& operator=(const ParserState&) = delete;

  void PushParserNoKey(BaseParser& parser);

  void PushParser(BaseParser& parser, std::string_view key);
  void PushParser(BaseParser& parser, size_t key);

  void ProcessInput(std::string_view sw);

  void PopMe(BaseParser& parser);

  [[noreturn]] void ThrowError(const std::string& err_msg);

 private:
  std::string GetCurrentPath() const;

  BaseParser& GetTopParser() const;

  struct Impl;
  utils::FastPimpl<Impl, 792, 8> impl_;

  friend class ParserHandler;
};

}  // namespace formats::json::parser
