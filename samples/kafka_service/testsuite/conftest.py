import pytest

# /// [Kafka service sample - kafka testsuite include]

pytest_plugins = ['pytest_userver.plugins.kafka']

# /// [Kafka service sample - kafka testsuite include]


# /// [Kafka service sample - secdist]
@pytest.fixture(scope='session')
def service_env(kafka_secdist) -> dict:
    """
    Note: kafka_secist fixture generates the secdist config

    Expected secdist format is:
    "kafka_settings": {
        "<kafka-component-name>": {
            "brokers": "<brokers comma-separated endpoint list>",
            "username": "SASL2 username (may be empty if use PLAINTEXT)",
            "password": "SASL2 password (may be empty if use PLAINTEXT)"
        }
    }
    """

    return {'SECDIST_CONFIG': kafka_secdist}
    # /// [Kafka service sample - secdist]


@pytest.fixture(scope='session')
def kafka_local(_patched_bootstrap_servers_internal):
    """
    Note: Should not be used, if using basic Kafka testsuite plugin.

    kafka_local is testsuite's fixture that is redefined
    to use custom Kafka cluster,
    but with kafka_producer and kafka_consumer fixtures.
    Here is only needed for internal testing purposes.
    """
    return _patched_bootstrap_servers_internal
