#include <userver/server/handlers/fallback_handlers.hpp>

#include <map>
#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

const std::string& ToString(FallbackHandler fallback) {
  static const std::string kImplicitOptions = "implicit-http-options";
  static const std::string kUnknown = "unknown";

  switch (fallback) {
    case FallbackHandler::kImplicitOptions:
      return kImplicitOptions;
  }
  return kUnknown;
}

namespace {
std::map<std::string, FallbackHandler> InitFallbackHandlerNames() {
  std::map<std::string, FallbackHandler> names;
  for (auto fallback : {FallbackHandler::kImplicitOptions}) {
    names[ToString(fallback)] = fallback;
  }
  return names;
}
}  // namespace

FallbackHandler FallbackHandlerFromString(const std::string& fallback_str) {
  static const std::map<std::string, FallbackHandler> fallback_map =
      InitFallbackHandlerNames();
  try {
    return fallback_map.at(fallback_str);
  } catch (std::exception& ex) {
    throw std::runtime_error("can't parse FallbackHandler from string '" +
                             fallback_str + '\'');
  }
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
