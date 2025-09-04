import pytest

pytest_plugins = ['testsuite.pytest_plugin']


def pytest_addoption(parser) -> None:
    group = parser.getgroup('userver')
    group.addoption('--test-binary', type=str, help='Path to build utility.')


@pytest.fixture(name='service_binary', scope='session')
def _service_binary(pytestconfig):
    return pytestconfig.option.test_binary
