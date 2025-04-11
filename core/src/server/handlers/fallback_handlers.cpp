#include <userver/server/handlers/fallback_handlers.hpp>

#include <stdexcept>

#include <userver/utils/algo.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

constexpr utils::TrivialBiMap kFallbackMap = [](auto selector) {
    return selector().Case("implicit-http-options", FallbackHandler::kImplicitOptions);
};

}

std::string_view ToString(FallbackHandler fallback) {
    return kFallbackMap.TryFindBySecond(fallback).value_or(utils::StringLiteral{"unknown"});
}

FallbackHandler FallbackHandlerFromString(std::string_view fallback_str) {
    const auto opt_value = kFallbackMap.TryFindByFirst(fallback_str);
    if (opt_value) {
        return *opt_value;
    }

    throw std::runtime_error(utils::StrCat("can't parse FallbackHandler from string '", fallback_str, "'"));
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
