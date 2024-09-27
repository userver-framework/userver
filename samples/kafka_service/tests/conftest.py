import pytest

pytest_plugins = ["pytest_userver.plugins.kafka"]


@pytest.fixture(scope="session")
def service_env(kafka_secdist):
    return {"SECDIST_CONFIG": kafka_secdist}
