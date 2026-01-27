#include <userver/kafka/impl/consumer_params.hpp>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

MessageKeyLogFormat Parse(const yaml_config::YamlConfig& config, formats::parse::To<MessageKeyLogFormat>) {
    auto log_format = config.As<std::string>("plaintext");
    if (log_format == "plaintext") {
        return MessageKeyLogFormat::kPlainText;
    }
    if (log_format == "hex") {
        return MessageKeyLogFormat::kHex;
    }

    throw std::runtime_error("Unknown message_key_log_format value: " + log_format);
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
