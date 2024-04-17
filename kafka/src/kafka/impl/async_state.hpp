#pragma once

#include <string>

#include <cppkafka/message_builder.h>

#include <userver/engine/future.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

struct AsyncState final {
  explicit AsyncState(std::string topic_name) : builder(std::move(topic_name)) {
    builder.user_data(&promise);
  }

  engine::Promise<cppkafka::Error> promise{};
  cppkafka::MessageBuilder builder;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
