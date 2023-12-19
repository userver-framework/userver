# /// [pytest]
import pytest


def pytest_addoption(parser) -> None:
    group = parser.getgroup('userver')
    group.addoption('--test-binary', type=str, help='Path to build utility.')


@pytest.fixture(scope='session')
def path_to_json2yaml(pytestconfig):
    return pytestconfig.option.test_binary
    # /// [pytest]
