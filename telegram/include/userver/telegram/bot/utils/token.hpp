#pragma once

#include <string_view>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

class Token {
 public:
  explicit Token(std::string_view token);

  std::string_view GetToken() const;
  std::string_view GetHiddenToken() const;

 private:
  static std::string MakeHiddenToken(std::string_view token);

  const std::string token_;
  const std::string hidden_token_;
};

}  // namespace telegram::bot

USERVER_NAMESPACE_END
