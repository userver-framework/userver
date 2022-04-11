import pathlib
import types

import pytest
import yaml

USERVER_CONFIG_HOOKS = [
    'userver_config_base',
    'userver_config_logging',
    'userver_config_testsuite',
]


class UserverConfigPlugin:
    def __init__(self):
        self._config_hooks = []

    @property
    def userver_config_hooks(self):
        return self._config_hooks

    def pytest_plugin_registered(self, plugin, manager):
        if not isinstance(plugin, types.ModuleType):
            return
        uhooks = getattr(plugin, 'USERVER_CONFIG_HOOKS', None)
        if uhooks is not None:
            self._config_hooks.extend(uhooks)


def pytest_configure(config):
    config.pluginmanager.register(UserverConfigPlugin(), 'userver_config')


def pytest_addoption(parser) -> None:
    group = parser.getgroup('userver-config')
    group.addoption(
        '--service-config',
        type=pathlib.Path,
        help='Path to service.yaml file.',
        required=True,
    )


@pytest.fixture(scope='session')
def service_config_path(
        pytestconfig, tmp_path_factory, service_config_yaml,
) -> pathlib.Path:
    destination = tmp_path_factory.mktemp(
        pytestconfig.option.service_binary.name,
    )
    dst_path = destination / 'config.yaml'
    dst_path.write_text(yaml.dump(service_config_yaml))
    return dst_path


@pytest.fixture(scope='session')
def service_config_yaml(pytestconfig, request) -> pathlib.Path:
    config_vars: dict = {}
    with pytestconfig.option.service_config.open('rt') as fp:
        config_yaml = yaml.safe_load(fp)

    plugin = pytestconfig.pluginmanager.get_plugin('userver_config')
    for hook in plugin.userver_config_hooks:
        if not callable(hook):
            hook_func = request.getfixturevalue(hook)
        else:
            hook_func = hook
        hook_func(config_yaml, config_vars)

    return config_yaml


@pytest.fixture(scope='session')
def userver_config_base(
        pytestconfig,
        build_dir: pathlib.Path,
        service_source_dir: pathlib.Path,
):
    def _patch_config(config_yaml, config_vars):
        if 'config_vars' in config_yaml:
            config_yaml['config_vars'] = str(
                service_source_dir.joinpath('config_vars.yaml'),
            )

        components = config_yaml['components_manager']['components']
        server = components['server']
        server['listener']['port'] = pytestconfig.option.test_service_port

        if 'listener-monitor' in server:
            server['listener-monitor'][
                'port'
            ] = pytestconfig.option.test_monitor_port

    return _patch_config


@pytest.fixture(scope='session')
def userver_config_logging():
    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        if 'logging' in components:
            components['logging']['loggers'] = {
                'default': {
                    'file_path': '@stderr',
                    'level': 'debug',
                    'overflow_behavior': 'discard',
                },
            }

    return _patch_config


@pytest.fixture(scope='session')
def userver_config_testsuite(mockserver_info):
    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        if 'tests-control' in components:
            components['tests-control']['testpoint-url'] = mockserver_info.url(
                'testpoint',
            )

    return _patch_config
