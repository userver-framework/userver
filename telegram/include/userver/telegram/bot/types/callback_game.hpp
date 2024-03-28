#pragma once

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief A placeholder, currently holds no information.
/// @note Use BotFather to set up your game.
/// @see https://core.telegram.org/bots/api#callbackgame
struct CallbackGame {};

CallbackGame Parse(const formats::json::Value& json,
                   formats::parse::To<CallbackGame>);

formats::json::Value Serialize(const CallbackGame& callback_game,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
