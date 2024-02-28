#pragma once

#include <string>
#include <string_view>

#include <userver/server/handlers/handler_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares::misc {

std::string_view CutTrailingSlash(
    const std::string& meta_type,
    handlers::UrlTrailingSlashOption trailing_slash);

}

USERVER_NAMESPACE_END
