#include <userver/server/handlers/auth/digest/exception.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

MissingDirectivesException::MissingDirectivesException(
    std::vector<std::string>&& missing_directives)
    : Exception(fmt::format("Mandatory '{}' directives is missing",
                            fmt::join(missing_directives, ", "))),
      missing_directives_(std::move(missing_directives)) {}

const std::vector<std::string>&
MissingDirectivesException::GetMissingDirectives() const noexcept {
  return missing_directives_;
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
