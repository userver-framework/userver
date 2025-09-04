#include <userver/kafka/utest/kafka_fixture.hpp>

#include <optional>

#include <userver/engine/subprocess/environment_variables.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using namespace std::chrono_literals;

class ConfigurationTest : public kafka::utest::KafkaCluster {};

using ConfigurationDeathTest = ConfigurationTest;

}  // namespace

UTEST_F(ConfigurationTest, Producer) {
    std::optional<kafka::impl::Configuration> configuration;
    UEXPECT_NO_THROW(configuration.emplace(MakeProducerConfiguration("kafka-producer")));

    const kafka::impl::ProducerConfiguration default_producer{};
    EXPECT_EQ(configuration->GetOption("client.id"), default_producer.common.client_id);
    EXPECT_EQ(
        configuration->GetOption("topic.metadata.refresh.interval.ms"),
        std::to_string(default_producer.common.topic_metadata_refresh_interval.count())
    );
    EXPECT_EQ(
        configuration->GetOption("metadata.max.age.ms"),
        std::to_string(default_producer.common.metadata_max_age.count())
    );
    EXPECT_EQ(configuration->GetOption("security.protocol"), "plaintext");
    EXPECT_EQ(
        configuration->GetOption("delivery.timeout.ms"), std::to_string(default_producer.delivery_timeout.count())
    );
    EXPECT_EQ(
        configuration->GetOption("queue.buffering.max.ms"), std::to_string(default_producer.queue_buffering_max.count())
    );
    EXPECT_EQ(configuration->GetOption("enable.idempotence"), default_producer.enable_idempotence ? "true" : "false");
    EXPECT_EQ(
        configuration->GetOption("queue.buffering.max.messages"),
        std::to_string(default_producer.queue_buffering_max_messages)
    );
    EXPECT_EQ(
        configuration->GetOption("queue.buffering.max.kbytes"),
        std::to_string(default_producer.queue_buffering_max_kbytes)
    );
    EXPECT_EQ(configuration->GetOption("message.max.bytes"), std::to_string(default_producer.message_max_bytes));
    EXPECT_EQ(
        configuration->GetOption("message.send.max.retries"), std::to_string(default_producer.message_send_max_retries)
    );
    EXPECT_EQ(configuration->GetOption("retry.backoff.ms"), std::to_string(default_producer.retry_backoff.count()));
    EXPECT_EQ(
        configuration->GetOption("retry.backoff.max.ms"), std::to_string(default_producer.retry_backoff_max.count())
    );
}

UTEST_F(ConfigurationTest, ProducerNonDefault) {
    kafka::impl::ProducerConfiguration producer_configuration{};
    producer_configuration.common.topic_metadata_refresh_interval = 10ms;
    producer_configuration.common.metadata_max_age = 30ms;
    producer_configuration.common.client_id = "test-client";
    producer_configuration.delivery_timeout = 37ms;
    producer_configuration.queue_buffering_max = 7ms;
    producer_configuration.enable_idempotence = true;
    producer_configuration.queue_buffering_max_messages = 33;
    producer_configuration.queue_buffering_max_kbytes = 55;
    producer_configuration.message_max_bytes = 3333;
    producer_configuration.message_send_max_retries = 3;
    producer_configuration.retry_backoff = 200ms;
    producer_configuration.retry_backoff_max = 2000ms;
    producer_configuration.rd_kafka_options["session.timeout.ms"] = "3600000";

    std::optional<kafka::impl::Configuration> configuration;
    UEXPECT_NO_THROW(configuration.emplace(MakeProducerConfiguration("kafka-producer", producer_configuration)));

    EXPECT_EQ(configuration->GetOption("client.id"), "test-client");
    EXPECT_EQ(configuration->GetOption("topic.metadata.refresh.interval.ms"), "10");
    EXPECT_EQ(configuration->GetOption("metadata.max.age.ms"), "30");
    EXPECT_EQ(configuration->GetOption("security.protocol"), "plaintext");
    EXPECT_EQ(configuration->GetOption("delivery.timeout.ms"), "37");
    EXPECT_EQ(configuration->GetOption("queue.buffering.max.ms"), "7");
    EXPECT_EQ(configuration->GetOption("enable.idempotence"), "true");
    EXPECT_EQ(configuration->GetOption("queue.buffering.max.messages"), "33");
    EXPECT_EQ(configuration->GetOption("queue.buffering.max.kbytes"), "55");
    EXPECT_EQ(configuration->GetOption("message.max.bytes"), "3333");
    EXPECT_EQ(configuration->GetOption("message.send.max.retries"), "3");
    EXPECT_EQ(configuration->GetOption("retry.backoff.ms"), "200");
    EXPECT_EQ(configuration->GetOption("retry.backoff.max.ms"), "2000");
    EXPECT_EQ(configuration->GetOption("session.timeout.ms"), "3600000");
}

UTEST_F(ConfigurationTest, Consumer) {
    std::optional<kafka::impl::Configuration> configuration;
    UEXPECT_NO_THROW(configuration.emplace(MakeConsumerConfiguration("kafka-consumer")));

    const kafka::impl::ConsumerConfiguration default_consumer{};
    EXPECT_EQ(configuration->GetOption("client.id"), default_consumer.common.client_id);
    EXPECT_EQ(
        configuration->GetOption("topic.metadata.refresh.interval.ms"),
        std::to_string(default_consumer.common.topic_metadata_refresh_interval.count())
    );
    EXPECT_EQ(
        configuration->GetOption("metadata.max.age.ms"),
        std::to_string(default_consumer.common.metadata_max_age.count())
    );
    EXPECT_EQ(configuration->GetOption("security.protocol"), "plaintext");
    EXPECT_EQ(configuration->GetOption("group.id"), "test-group");
    EXPECT_EQ(configuration->GetOption("auto.offset.reset"), default_consumer.auto_offset_reset);
    EXPECT_EQ(configuration->GetOption("enable.auto.commit"), "false");
}

UTEST_F(ConfigurationTest, ConsumerNonDefault) {
    kafka::impl::ConsumerConfiguration consumer_configuration{};
    consumer_configuration.common.topic_metadata_refresh_interval = 10ms;
    consumer_configuration.common.metadata_max_age = 30ms;
    consumer_configuration.common.client_id = "test-client";
    consumer_configuration.auto_offset_reset = "largest";
    consumer_configuration.rd_kafka_options["socket.keepalive.enable"] = "true";

    std::optional<kafka::impl::Configuration> configuration;
    UEXPECT_NO_THROW(configuration.emplace(MakeConsumerConfiguration("kafka-consumer", consumer_configuration)));

    EXPECT_EQ(configuration->GetOption("topic.metadata.refresh.interval.ms"), "10");
    EXPECT_EQ(configuration->GetOption("client.id"), "test-client");
    EXPECT_EQ(configuration->GetOption("metadata.max.age.ms"), "30");
    EXPECT_EQ(configuration->GetOption("security.protocol"), "plaintext");
    EXPECT_EQ(configuration->GetOption("group.id"), "test-group");
    EXPECT_EQ(configuration->GetOption("auto.offset.reset"), "largest");
    EXPECT_EQ(configuration->GetOption("socket.keepalive.enable"), "true");
}

UTEST_F(ConfigurationTest, ProducerSaslSsl) {
    kafka::impl::ProducerConfiguration producer_configuration{};
    producer_configuration.security.security_protocol = kafka::impl::SecurityConfiguration::SaslSsl{
        /*security_mechanism=*/"SCRAM-SHA-512",
        /*ssl_ca_location=*/"probe",
    };

    kafka::impl::Secret secrets;
    secrets.credentials = kafka::impl::Secret::SaslCredentials{
        kafka::impl::Secret::SecretType{"username"},
        kafka::impl::Secret::SecretType{"password"},
    };

    std::optional<kafka::impl::Configuration> configuration;
    UEXPECT_NO_THROW(configuration.emplace(MakeProducerConfiguration("kafka-producer", producer_configuration, secrets))
    );

    EXPECT_EQ(configuration->GetOption("security.protocol"), "sasl_ssl");
    EXPECT_EQ(configuration->GetOption("sasl.mechanism"), "SCRAM-SHA-512");
    EXPECT_EQ(configuration->GetOption("sasl.username"), "username");
    EXPECT_EQ(configuration->GetOption("sasl.password"), "password");
    EXPECT_EQ(configuration->GetOption("ssl.ca.location"), "probe");
}

UTEST_F(ConfigurationTest, ProducerSaslPlaintext) {
    kafka::impl::ProducerConfiguration producer_configuration{};
    producer_configuration.security.security_protocol = kafka::impl::SecurityConfiguration::SaslPlaintext{
        /*security_mechanism=*/"SCRAM-SHA-512",
    };

    kafka::impl::Secret secrets;
    secrets.credentials = kafka::impl::Secret::SaslCredentials{
        kafka::impl::Secret::SecretType{"username"},
        kafka::impl::Secret::SecretType{"password"},
    };

    std::optional<kafka::impl::Configuration> configuration;
    UEXPECT_NO_THROW(configuration.emplace(MakeProducerConfiguration("kafka-producer", producer_configuration, secrets))
    );

    EXPECT_EQ(configuration->GetOption("security.protocol"), "sasl_plaintext");
    EXPECT_EQ(configuration->GetOption("sasl.mechanism"), "SCRAM-SHA-512");
    EXPECT_EQ(configuration->GetOption("sasl.username"), "username");
    EXPECT_EQ(configuration->GetOption("sasl.password"), "password");
}

UTEST_F(ConfigurationTest, ConsumerSaslSsl) {
    kafka::impl::ConsumerConfiguration consumer_configuration{};
    consumer_configuration.security.security_protocol = kafka::impl::SecurityConfiguration::SaslSsl{
        /*security_mechanism=*/"SCRAM-SHA-512",
        /*ssl_ca_location=*/"/etc/ssl/cert.ca",
    };

    kafka::impl::Secret secrets;
    secrets.credentials = kafka::impl::Secret::SaslCredentials{
        kafka::impl::Secret::SecretType{"username"},
        kafka::impl::Secret::SecretType{"password"},
    };

    std::optional<kafka::impl::Configuration> configuration;
    UEXPECT_NO_THROW(configuration.emplace(MakeConsumerConfiguration("kafka-consumer", consumer_configuration, secrets))
    );

    EXPECT_EQ(configuration->GetOption("security.protocol"), "sasl_ssl");
    EXPECT_EQ(configuration->GetOption("sasl.mechanism"), "SCRAM-SHA-512");
    EXPECT_EQ(configuration->GetOption("sasl.username"), "username");
    EXPECT_EQ(configuration->GetOption("sasl.password"), "password");
    EXPECT_EQ(configuration->GetOption("ssl.ca.location"), "/etc/ssl/cert.ca");
}

UTEST_F(ConfigurationTest, ConsumerSaslPlaintext) {
    kafka::impl::ConsumerConfiguration consumer_configuration{};
    consumer_configuration.security.security_protocol = kafka::impl::SecurityConfiguration::SaslPlaintext{
        /*security_mechanism=*/"SCRAM-SHA-512",
    };

    kafka::impl::Secret secrets;
    secrets.credentials = kafka::impl::Secret::SaslCredentials{
        kafka::impl::Secret::SecretType{"username"},
        kafka::impl::Secret::SecretType{"password"},
    };

    std::optional<kafka::impl::Configuration> configuration;
    UEXPECT_NO_THROW(configuration.emplace(MakeConsumerConfiguration("kafka-consumer", consumer_configuration, secrets))
    );

    EXPECT_EQ(configuration->GetOption("security.protocol"), "sasl_plaintext");
    EXPECT_EQ(configuration->GetOption("sasl.mechanism"), "SCRAM-SHA-512");
    EXPECT_EQ(configuration->GetOption("sasl.username"), "username");
    EXPECT_EQ(configuration->GetOption("sasl.password"), "password");
}

UTEST_F(ConfigurationTest, ProducerSsl) {
    kafka::impl::ProducerConfiguration producer_configuration{};
    producer_configuration.security.security_protocol = kafka::impl::SecurityConfiguration::Ssl{
        /*ssl_ca_location=*/"/etc/ssl/ca.crt",
    };

    kafka::impl::Secret secrets;
    secrets.credentials = kafka::impl::Secret::SslCredentials{
        kafka::impl::Secret::SecretType{"/etc/ssl/client.crt"},
        kafka::impl::Secret::SecretType{"/etc/ssl/client.key"},
        kafka::impl::Secret::SecretType{"password123"},
    };

    std::optional<kafka::impl::Configuration> configuration;
    UEXPECT_NO_THROW(configuration.emplace(MakeProducerConfiguration("kafka-producer", producer_configuration, secrets))
    );

    EXPECT_EQ(configuration->GetOption("security.protocol"), "ssl");
    EXPECT_EQ(configuration->GetOption("ssl.ca.location"), "/etc/ssl/ca.crt");
    EXPECT_EQ(configuration->GetOption("ssl.certificate.location"), "/etc/ssl/client.crt");
    EXPECT_EQ(configuration->GetOption("ssl.key.location"), "/etc/ssl/client.key");
    EXPECT_EQ(configuration->GetOption("ssl.key.password"), "password123");
}

UTEST_F(ConfigurationTest, ConsumerSSL) {
    kafka::impl::ConsumerConfiguration consumer_configuration{};
    consumer_configuration.security.security_protocol = kafka::impl::SecurityConfiguration::Ssl{
        /*ssl_ca_location=*/"/etc/ssl/ca.crt",
    };

    kafka::impl::Secret secrets;
    secrets.credentials = kafka::impl::Secret::SslCredentials{
        kafka::impl::Secret::SecretType{"/etc/ssl/client.crt"},
        kafka::impl::Secret::SecretType{"/etc/ssl/client.key"},
    };

    std::optional<kafka::impl::Configuration> configuration;
    UEXPECT_NO_THROW(configuration.emplace(MakeConsumerConfiguration("kafka-consumer", consumer_configuration, secrets))
    );

    EXPECT_EQ(configuration->GetOption("security.protocol"), "ssl");
    EXPECT_EQ(configuration->GetOption("ssl.ca.location"), "/etc/ssl/ca.crt");
    EXPECT_EQ(configuration->GetOption("ssl.certificate.location"), "/etc/ssl/client.crt");
    EXPECT_EQ(configuration->GetOption("ssl.key.location"), "/etc/ssl/client.key");
}

UTEST_F(ConfigurationTest, IncorrectComponentName) {
    UEXPECT_THROW(MakeProducerConfiguration("producer"), std::runtime_error);
    UEXPECT_THROW(MakeConsumerConfiguration("consumer"), std::runtime_error);
}

UTEST_F(ConfigurationTest, ConsumerResolveGroupId) {
    kafka::impl::ConsumerConfiguration consumer_configuration{};
    consumer_configuration.group_id = "test-group-{pod_name}";
    consumer_configuration.env_pod_name = "ENVIRONMENT_VARIABLE_NAME";

    const engine::subprocess::EnvironmentVariablesScope scope{};
    engine::subprocess::SetEnvironmentVariable(
        "ENVIRONMENT_VARIABLE_NAME", "pod-example-com", engine::subprocess::Overwrite::kAllowed
    );

    std::optional<kafka::impl::Configuration> configuration;
    UEXPECT_NO_THROW(configuration.emplace(MakeConsumerConfiguration("kafka-consumer", consumer_configuration)));

    EXPECT_EQ(configuration->GetOption("group.id"), "test-group-pod-example-com");
}

UTEST_F_DEATH(ConfigurationDeathTest, ContradictorySecurityConfiguration) {
    kafka::impl::ProducerConfiguration sasl_ssl{};
    sasl_ssl.security.security_protocol = kafka::impl::SecurityConfiguration::SaslSsl{
        /*security_mechanism=*/"SCRAM-SHA-512",
        /*ssl_ca_location=*/"probe",
    };
    kafka::impl::ProducerConfiguration sasl_plaintext{};
    sasl_plaintext.security.security_protocol = kafka::impl::SecurityConfiguration::SaslPlaintext{
        /*security_mechanism=*/"SCRAM-SHA-512",
    };
    kafka::impl::ConsumerConfiguration ssl{};
    ssl.security.security_protocol = kafka::impl::SecurityConfiguration::Ssl{
        /*ssl_ca_location=*/"/etc/ssl/ca.crt",
    };

    kafka::impl::Secret secrets_none;
    kafka::impl::Secret secrets_sasl;
    secrets_sasl.credentials = kafka::impl::Secret::SaslCredentials{
        kafka::impl::Secret::SecretType{"username"},
        kafka::impl::Secret::SecretType{"password"},
    };
    kafka::impl::Secret secrets_ssl;
    secrets_ssl.credentials = kafka::impl::Secret::SslCredentials{
        kafka::impl::Secret::SecretType{"/etc/ssl/client.crt"},
        kafka::impl::Secret::SecretType{"/etc/ssl/client.key"},
        kafka::impl::Secret::SecretType{"password123"},
    };

    std::optional<kafka::impl::Configuration> configuration;
#ifdef NDEBUG
    UEXPECT_THROW(
        configuration.emplace(MakeProducerConfiguration("kafka-producer", sasl_ssl, secrets_none)), std::exception
    );
    UEXPECT_THROW(
        configuration.emplace(MakeProducerConfiguration("kafka-producer", sasl_ssl, secrets_ssl)), std::exception
    );
    UEXPECT_THROW(
        configuration.emplace(MakeProducerConfiguration("kafka-producer", sasl_plaintext, secrets_none)), std::exception
    );
    UEXPECT_THROW(
        configuration.emplace(MakeProducerConfiguration("kafka-producer", sasl_plaintext, secrets_ssl)), std::exception
    );
    UEXPECT_THROW(
        configuration.emplace(MakeConsumerConfiguration("kafka-consumer", ssl, secrets_none)), std::exception
    );
    UEXPECT_THROW(
        configuration.emplace(MakeConsumerConfiguration("kafka-consumer", ssl, secrets_sasl)), std::exception
    );
#else
    UEXPECT_DEATH(
        configuration.emplace(MakeProducerConfiguration("kafka-producer", sasl_ssl, secrets_none)),
        "For 'SASL_SSL' security protocol, 'username' and 'password' are required in secdist 'kafka_settings'"
    );
    UEXPECT_DEATH(
        configuration.emplace(MakeProducerConfiguration("kafka-producer", sasl_ssl, secrets_ssl)),
        "For 'SASL_SSL' security protocol, 'username' and 'password' are required in secdist 'kafka_settings'"
    );
    UEXPECT_DEATH(
        configuration.emplace(MakeProducerConfiguration("kafka-producer", sasl_plaintext, secrets_none)),
        "For 'SASL_PLAINTEXT' security protocol, 'username' and 'password' are required in secdist 'kafka_settings'"
    );
    UEXPECT_DEATH(
        configuration.emplace(MakeProducerConfiguration("kafka-producer", sasl_plaintext, secrets_ssl)),
        "For 'SASL_PLAINTEXT' security protocol, 'username' and 'password' are required in secdist 'kafka_settings'"
    );
    UEXPECT_DEATH(
        configuration.emplace(MakeConsumerConfiguration("kafka-consumer", ssl, secrets_none)),
        "For 'SSL' security protocol, 'ssl_certificate_location', 'ssl_key_location' and optionally 'ssl_key_password' "
        "are required in secdist 'kafka_settings'"
    );
    UEXPECT_DEATH(
        configuration.emplace(MakeConsumerConfiguration("kafka-consumer", ssl, secrets_sasl)),
        "For 'SSL' security protocol, 'ssl_certificate_location', 'ssl_key_location' and optionally 'ssl_key_password' "
        "are required in secdist 'kafka_settings'"
    );
#endif
}

UTEST_F(ConfigurationTest, BrokerSecrets) {
    const auto make_kafka_settings = [](formats::json::Value component_settings) {
        return formats::json::MakeObject(
            "kafka_settings", formats::json::MakeObject("kafka-client", std::move(component_settings))
        );
    };

    const auto only_brokers = formats::json::MakeObject("brokers", "localhost:1111");
    const auto sasl = formats::json::MakeObject(
        "brokers",
        "localhost:1111",
        //
        "username",
        "user",
        //
        "password",
        "pass"
        //
    );
    const auto ssl = formats::json::MakeObject(
        "brokers",
        "localhost:1111",
        //
        "ssl_certificate_location",
        "/etc/ssl/client.crt",
        //
        "ssl_key_location",
        "/etc/ssl/client.key",
        //
        "ssl_key_password",
        "pass"
        //
    );
    const auto ssl_no_password = formats::json::MakeObject(
        "brokers",
        "localhost:1111",
        //
        "ssl_certificate_location",
        "/etc/ssl/client.crt",
        //
        "ssl_key_location",
        "/etc/ssl/client.key"
        //
    );

    std::optional<kafka::impl::BrokerSecrets> broker_secrets;
    std::optional<kafka::impl::Secret> secret;

    {
        UEXPECT_NO_THROW(broker_secrets.emplace(make_kafka_settings(only_brokers)));
        UEXPECT_NO_THROW(secret.emplace(broker_secrets->GetSecretByComponentName("kafka-client")));
        ASSERT_TRUE(std::holds_alternative<std::monostate>(secret->credentials));
        EXPECT_EQ(secret->brokers, "localhost:1111");
    }
    {
        UEXPECT_NO_THROW(broker_secrets.emplace(make_kafka_settings(sasl)));
        UEXPECT_NO_THROW(secret.emplace(broker_secrets->GetSecretByComponentName("kafka-client")));
        ASSERT_TRUE(std::holds_alternative<kafka::impl::Secret::SaslCredentials>(secret->credentials));
        EXPECT_EQ(secret->brokers, "localhost:1111");
        EXPECT_EQ(std::get<kafka::impl::Secret::SaslCredentials>(secret->credentials).username.GetUnderlying(), "user");
        EXPECT_EQ(std::get<kafka::impl::Secret::SaslCredentials>(secret->credentials).password.GetUnderlying(), "pass");
    }
    {
        UEXPECT_NO_THROW(broker_secrets.emplace(make_kafka_settings(ssl)));
        UEXPECT_NO_THROW(secret.emplace(broker_secrets->GetSecretByComponentName("kafka-client")));
        ASSERT_TRUE(std::holds_alternative<kafka::impl::Secret::SslCredentials>(secret->credentials));
        EXPECT_EQ(secret->brokers, "localhost:1111");
        EXPECT_EQ(
            std::get<kafka::impl::Secret::SslCredentials>(secret->credentials).ssl_certificate_location.GetUnderlying(),
            "/etc/ssl/client.crt"
        );
        EXPECT_EQ(
            std::get<kafka::impl::Secret::SslCredentials>(secret->credentials).ssl_key_location.GetUnderlying(),
            "/etc/ssl/client.key"
        );
        EXPECT_EQ(
            std::get<kafka::impl::Secret::SslCredentials>(secret->credentials).ssl_key_password->GetUnderlying(), "pass"
        );
    }
    {
        UEXPECT_NO_THROW(broker_secrets.emplace(make_kafka_settings(ssl_no_password)));
        UEXPECT_NO_THROW(secret.emplace(broker_secrets->GetSecretByComponentName("kafka-client")));
        ASSERT_TRUE(std::holds_alternative<kafka::impl::Secret::SslCredentials>(secret->credentials));
        EXPECT_EQ(secret->brokers, "localhost:1111");
        EXPECT_EQ(
            std::get<kafka::impl::Secret::SslCredentials>(secret->credentials).ssl_certificate_location.GetUnderlying(),
            "/etc/ssl/client.crt"
        );
        EXPECT_EQ(
            std::get<kafka::impl::Secret::SslCredentials>(secret->credentials).ssl_key_location.GetUnderlying(),
            "/etc/ssl/client.key"
        );
        EXPECT_FALSE(std::get<kafka::impl::Secret::SslCredentials>(secret->credentials).ssl_key_password.has_value());
    }
}

USERVER_NAMESPACE_END
