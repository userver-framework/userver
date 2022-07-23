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
    return pytestconfig.option.service_source_dir


@pytest.fixture(scope='session')
def sample_config_hook(service_source_dir):
    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        if 'secdist' in components:
            components['secdist']['config'] = str(
                pathlib.Path(service_source_dir).joinpath('secure_data.json'),
            )

    return _patch_config
