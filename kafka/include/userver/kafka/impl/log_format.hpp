#pragma once

#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

/// @brief Specifies the logging format for the message key.
enum class MessageKeyLogFormat {
    kPlainText, ///< Log message key as is
    kHex, ///< Log message key in hex
};

MessageKeyLogFormat Parse(const yaml_config::YamlConfig& config, formats::parse::To<MessageKeyLogFormat>);

}  // namespace kafka::impl

USERVER_NAMESPACE_END
