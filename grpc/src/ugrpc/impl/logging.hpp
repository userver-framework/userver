#pragma once

#include <string>

#include <google/protobuf/message.h>

#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

struct MessageLoggingOptions {
    logging::Level log_level{logging::Level::kDebug};
    std::size_t max_size{512};
    bool trim_secrets{true};
};

std::string GetMessageForLogging(const google::protobuf::Message& message, MessageLoggingOptions options = {});

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
