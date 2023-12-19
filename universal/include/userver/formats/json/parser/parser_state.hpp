#pragma once

#include <string>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

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

  void PushParser(BaseParser& parser);

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

USERVER_NAMESPACE_END
