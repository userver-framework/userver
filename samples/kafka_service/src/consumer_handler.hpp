#pragma once

#include <string_view>

/// [Kafka service sample - consumer include]

#include <userver/kafka/consumer_scope.hpp>

/// [Kafka service sample - consumer include]

#include <userver/components/component_base.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace kafka_sample {

/// [Kafka service sample - consumer component declaration]
class ConsumerHandler final : public components::ComponentBase {
public:
    static constexpr std::string_view kName{"consumer-handler"};

    ConsumerHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

private:
    // Subscriptions must be the last fields! Add new fields above this comment.
    kafka::ConsumerScope consumer_;
};
/// [Kafka service sample - consumer component declaration]

}  // namespace kafka_sample
