#include <userver/telegram/bot/types/game.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
Game Parse(const Value& data, formats::parse::To<Game>) {
  return Game{
    data["title"].template As<std::string>(),
    data["description"].template As<std::string>(),
    data["photo"].template As<std::vector<PhotoSize>>(),
    data["text"].template As<std::optional<std::string>>(),
    data["text_entities"].template As<std::optional<std::vector<MessageEntity>>>(),
    data["animation"].template As<std::unique_ptr<Animation>>()
  };
}

template <class Value>
Value Serialize(const Game& game, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["title"] = game.title;
  builder["description"] = game.description;
  builder["photo"] = game.photo;
  SetIfNotNull(builder, "text", game.text);
  SetIfNotNull(builder, "text_entities", game.text_entities);
  SetIfNotNull(builder, "animation", game.animation);
  return builder.ExtractValue();
}

}  // namespace impl

Game Parse(const formats::json::Value& json,
           formats::parse::To<Game> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Game& game,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(game, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
