#include <userver/server/handlers/auth/digest/digest_directives_parser.hpp>

#include <fmt/format.h>

#include <cctype>

#include <userver/logging/log.hpp>
#include <userver/server/handlers/auth/digest/digest_directives.hpp>
#include <userver/utils/exception.hpp>
#include <userver/utils/statistics/fmt.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

namespace {
const std::array<std::string, 5> kMandatoryDirectives = {
    directives::kRealm,     //
    directives::kNonce,     //
    directives::kResponse,  //
    directives::kUri,       //
    directives::kUsername};
}  // namespace

void Parser::ParseAuthInfo(std::string_view header_value) {
  enum class State {
    kStateSpace,
    kStateToken,
    kStateEquals,
    kStateValue,  // if Final then OK
    kStateValueQuoted,
    kStateValueEscape,
    kStateComma,  // if Final then OK
  };

  State state = State::kStateSpace;
  std::string token;
  std::string value;

  for (char delimiter : header_value) {
    switch (state) {
      case State::kStateSpace:
        if (std::isalnum(delimiter) || delimiter == '_' || delimiter == '-') {
          token += delimiter;
          state = State::kStateToken;
        } else if (std::isspace(delimiter)) {
          // Skip
        } else
          utils::LogErrorAndThrow("Invalid authentication information");
        break;

      case State::kStateToken:
        if (delimiter == '=') {
          state = State::kStateEquals;
        } else if (std::isalnum(delimiter) || delimiter == '_' ||
                   delimiter == '-') {
          token += delimiter;
        } else
          utils::LogErrorAndThrow("Invalid authentication information");
        break;

      case State::kStateEquals:
        if (std::isalnum(delimiter) || delimiter == '_') {
          value += delimiter;
          state = State::kStateValue;
        } else if (delimiter == '"') {
          state = State::kStateValueQuoted;
        } else
          utils::LogErrorAndThrow("Invalid authentication information");
        break;

      case State::kStateValueQuoted:
        if (delimiter == '\\') {
          state = State::kStateValueEscape;
        } else if (delimiter == '"') {
          directive_mapping[token] = std::move(value);
          token.clear();
          value.clear();
          state = State::kStateComma;
        } else {
          value += delimiter;
        }
        break;

      case State::kStateValueEscape:
        value += delimiter;
        state = State::kStateValueQuoted;
        break;

      case State::kStateValue:
        if (std::isspace(delimiter)) {
          directive_mapping[token] = std::move(value);
          token.clear();
          value.clear();
          state = State::kStateComma;
        } else if (delimiter == ',') {
          directive_mapping[token] = std::move(value);
          token.clear();
          value.clear();
          state = State::kStateSpace;
        } else {
          value += delimiter;
        }
        break;

      case State::kStateComma:
        if (delimiter == ',') {
          state = State::kStateSpace;
        } else if (std::isspace(delimiter)) {
          // Skip
        } else
          utils::LogErrorAndThrow("Invalid authentication information");
        break;
    }
  }

  if (state == State::kStateValue) {
    directive_mapping[token] = std::move(value);
  }

  if (state != State::kStateValue && state != State::kStateComma)
    utils::LogErrorAndThrow("Invalid authentication information");
}

ContextFromClient Parser::GetClientContext() {
  // Only checking mandatory directives
  // because according to RFC 2617,
  // "Any unrecognized directive MUST be ignored"
  for (const auto& dir : kMandatoryDirectives) {
    if (!directive_mapping.count(dir)) {
      utils::LogErrorAndThrow(fmt::format(
          "Mandatory {} directive is missing in Authentication header", dir));
    }
  }
  return Parse(directive_mapping);
}

}  // namespace server::handlers::auth::digest

USERVER_NAMESPACE_END
