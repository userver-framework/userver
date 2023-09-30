#include <userver/telegram/bot/utils/token.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

Token::Token(std::string_view token)
    : token_(token)
    , hidden_token_(MakeHiddenToken(token)) {}

std::string_view Token::GetToken() const {
  return token_;
}

std::string_view Token::GetHiddenToken() const {
  return hidden_token_;
}

std::string Token::MakeHiddenToken(std::string_view token) {
  std::string hidden_token{token};

  size_t pos = token.find(':');
  UINVARIANT(pos != std::string_view::npos, "Invalid bot token");  

  for (size_t i = pos + 10; i < hidden_token.size(); ++i) {
    hidden_token[i] = '*';
  }
  return hidden_token;
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
