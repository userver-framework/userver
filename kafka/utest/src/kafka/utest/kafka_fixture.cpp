#include <userver/kafka/utest/kafka_fixture.hpp>

#include <algorithm>
#include <tuple>

#include <fmt/format.h>

#include <userver/engine/single_use_event.hpp>
#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/utils/lazy_prvalue.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::utest {

namespace {

constexpr const char* kTestsuiteKafkaServerHost{"TESTSUITE_KAFKA_SERVER_HOST"};
constexpr const char* kDefaultKafkaServerHost{"localhost"};
constexpr const char* kTestsuiteKafkaServerPort{"TESTSUITE_KAFKA_SERVER_PORT"};
constexpr const char* kDefaultKafkaServerPort{"9099"};
constexpr const char* kRecipeKafkaBrokersList{"KAFKA_RECIPE_BROKER_LIST"};

std::string FetchBrokerList() {
    const auto env = engine::subprocess::GetCurrentEnvironmentVariablesPtr();

    if (const auto* brokers_list = env->GetValueOptional(kRecipeKafkaBrokersList)) {
        return *brokers_list;
    }

    std::string server_host{kDefaultKafkaServerHost};
    std::string server_port{kDefaultKafkaServerPort};
    const auto* host = env->GetValueOptional(kTestsuiteKafkaServerHost);
    if (host) {
        server_host = *host;
    }
    const auto* port = env->GetValueOptional(kTestsuiteKafkaServerPort);
    if (port) {
        server_port = *port;
    }
    return fmt::format("{}:{}", server_host, server_port);
}

impl::Secret MakeSecrets(std::string_view bootstrap_servers) {
    impl::Secret secrets{};
    secrets.brokers = bootstrap_servers;

    return secrets;
}

impl::ProducerConfiguration PatchDeliveryTimeout(impl::ProducerConfiguration configuration) {
    static const impl::ProducerConfiguration kDefaultProducerConfiguration{};

    if (configuration.delivery_timeout == kDefaultProducerConfiguration.delivery_timeout) {
        configuration.delivery_timeout = KafkaCluster::kDefaultTestProducerTimeout;
    }

    return configuration;
}

}  // namespace

bool operator==(const Message& lhs, const Message& rhs) {
    return std::tie(lhs.topic, lhs.key, lhs.payload, lhs.partition) ==
           std::tie(rhs.topic, rhs.key, rhs.payload, rhs.partition);
}

KafkaCluster::KafkaCluster() : bootstrap_servers_(FetchBrokerList()) {}

std::atomic<std::size_t> KafkaCluster::kTopicsCount{0};

std::string KafkaCluster::GenerateTopic() { return fmt::format("tt-{}", kTopicsCount.fetch_add(1)); }

std::vector<std::string> KafkaCluster::GenerateTopics(std::size_t count) {
    std::vector<std::string> topics{count};
    std::generate_n(topics.begin(), count, [this] { return GenerateTopic(); });

    return topics;
}

impl::Configuration KafkaCluster::MakeProducerConfiguration(
    const std::string& name,
    impl::ProducerConfiguration configuration,
    impl::Secret secrets
) {
    return impl::Configuration{name, configuration, AddBootstrapServers(secrets)};
}

impl::Configuration KafkaCluster::MakeConsumerConfiguration(
    const std::string& name,
    impl::ConsumerConfiguration configuration,
    impl::Secret secrets
) {
    if (configuration.group_id.empty()) {
        configuration.group_id = "test-group";
    }
    return impl::Configuration{name, configuration, AddBootstrapServers(secrets)};
}

Producer KafkaCluster::MakeProducer(const std::string& name, impl::ProducerConfiguration configuration) {
    return Producer{
        name,
        engine::current_task::GetTaskProcessor(),
        PatchDeliveryTimeout(std::move(configuration)),
        MakeSecrets(bootstrap_servers_)};
}

std::deque<Producer> KafkaCluster::MakeProducers(
    std::size_t count,
    std::function<std::string(std::size_t)> nameGenerator,
    impl::ProducerConfiguration configuration
) {
    std::deque<Producer> producers;
    for (std::size_t i{0}; i < count; ++i) {
        producers.emplace_back(utils::LazyPrvalue([&] { return MakeProducer(nameGenerator(i), configuration); }));
    }

    return producers;
}

void KafkaCluster::SendMessages(utils::span<const Message> messages) {
    const Producer producer = MakeProducer("kafka-producer");

    std::vector<engine::TaskWithResult<void>> results;
    results.reserve(messages.size());
    for (const auto& message : messages) {
        std::vector<HeaderView> header_views;
        header_views.reserve(message.headers.size());
        for (const auto& header : message.headers) {
            header_views.push_back(HeaderView{header.GetName(), header.GetValue()});
        }

        results.emplace_back(
            producer.SendAsync(message.topic, message.key, message.payload, message.partition, header_views)
        );
    }

    engine::WaitAllChecked(results);
}

impl::Consumer KafkaCluster::MakeConsumer(
    const std::string& name,
    const std::vector<std::string>& topics,
    impl::ConsumerConfiguration configuration,
    impl::ConsumerExecutionParams params
) {
    if (configuration.group_id.empty()) {
        configuration.group_id = "test-group";
    }
    return impl::Consumer{
        name,
        topics,
        engine::current_task::GetTaskProcessor(),
        engine::current_task::GetTaskProcessor(),
        engine::current_task::GetTaskProcessor(),
        configuration,
        MakeSecrets(bootstrap_servers_),
        std::move(params)};
}

std::vector<Message> KafkaCluster::ReceiveMessages(
    impl::Consumer& consumer,
    std::size_t expected_messages_count,
    bool commit_after_receive,
    std::optional<std::function<void(MessageBatchView)>> user_callback
) {
    auto consumer_scope = consumer.MakeConsumerScope();
    return ReceiveMessages(consumer_scope, expected_messages_count, commit_after_receive, std::move(user_callback));
}

std::vector<Message> KafkaCluster::ReceiveMessages(
    ConsumerScope& consumer_scope,
    std::size_t expected_messages_count,
    bool commit_after_receive,
    std::optional<std::function<void(MessageBatchView)>> user_callback
) {
    std::vector<Message> received_messages;

    engine::SingleUseEvent event;
    consumer_scope.Start([&received_messages,
                          expected_messages_count,
                          &event,
                          &consumer_scope,
                          &user_callback,
                          commit = commit_after_receive](MessageBatchView messages) {
        for (const auto& message : messages) {
            auto reader = message.GetHeaders();
            received_messages.emplace_back(kafka::utest::Message{
                message.GetTopic(),
                std::string{message.GetKey()},
                std::string{message.GetPayload()},
                message.GetPartition(),
                std::vector<kafka::OwningHeader>{reader.begin(), reader.end()}});
        }
        if (user_callback) {
            (*user_callback)(messages);
        }
        if (commit) {
            consumer_scope.AsyncCommit();
        }

        if (received_messages.size() == expected_messages_count) {
            event.Send();
        }
    });

    event.Wait();

    return received_messages;
}

impl::Secret KafkaCluster::AddBootstrapServers(impl::Secret secrets) const {
    secrets.brokers = bootstrap_servers_;

    return secrets;
}

}  // namespace kafka::utest

USERVER_NAMESPACE_END
