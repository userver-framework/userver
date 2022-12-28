import pathlib

import pytest

USERVER_CONFIG_HOOKS = ['sample_config_hook']


def pytest_addoption(parser) -> None:
    group = parser.getgroup('sample-service')
    group.addoption(
        '--service-source-dir',
        type=pathlib.Path,
        help='Path to service source directory.',
    )


@pytest.fixture(scope='session')
def service_source_dir(pytestconfig):
    """
    Returns the path to the service source directory that is set by command
    line `--service-source-dir` option.

    Override this fixture to change the way the path to the service
    source directory is detected by testsuite.

    @ingroup userver_testsuite_fixtures
    """
    return pytestconfig.option.service_source_dir


@pytest.fixture(scope='session')
def sample_config_hook(service_source_dir):
    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        if 'default-secdist-provider' in components:
            components['default-secdist-provider']['config'] = str(
                pathlib.Path(service_source_dir).joinpath('secure_data.json'),
            )

    return _patch_config
