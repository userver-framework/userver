#pragma once

#include <chrono>

#include <userver/chaotic/openapi/client/config.hpp>
#include <userver/dynamic_config/fwd.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/utils/default_dict.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

struct CommandControl {
    std::chrono::milliseconds timeout{};
    int attempts{};
};

CommandControl Parse(const formats::json::Value& value, formats::parse::To<CommandControl>);

using CommandControlDict = utils::DefaultDict<CommandControl>;

void ApplyConfig(clients::http::Request& request, const CommandControl& cc, const Config& config);

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
