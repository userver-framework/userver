import pytest


pytest_plugins = ["pytest_userver.plugins.kafka"]


@pytest.fixture(scope="session")
def kafka_local(bootstrap_servers):
    return bootstrap_servers


# /// [Kafka service sample - secdist]
@pytest.fixture(scope="session")
def service_env(kafka_secdist):
    return {"SECDIST_CONFIG": kafka_secdist}
    # /// [Kafka service sample - secdist]
