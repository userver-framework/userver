import pathlib
import types
import typing

import pytest
import yaml

USERVER_CONFIG_HOOKS = [
    'userver_config_base',
    'userver_config_logging',
    'userver_config_testsuite',
    'userver_config_fallback',
]


class BaseError(Exception):
    """Base class for exceptions of this module"""


class ConfigNotFoundError(BaseError):
    """Config parameter was not found and no default was provided"""


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


class UserverConfig(typing.NamedTuple):
    config_yaml: dict
    config_vars: dict


def pytest_configure(config):
    config.pluginmanager.register(UserverConfigPlugin(), 'userver_config')
    config.addinivalue_line('markers', 'config: per-test taxi config values')


def pytest_addoption(parser) -> None:
    group = parser.getgroup('userver-config')
    group.addoption(
        '--service-config',
        type=pathlib.Path,
        help='Path to service.yaml file.',
    )
    group.addoption(
        '--service-config-vars',
        type=pathlib.Path,
        help='Path to config_vars.yaml file.',
    )
    group.addoption(
        '--config-fallback',
        type=pathlib.Path,
        help='Path to config fallback file.',
    )


@pytest.fixture(scope='session')
def service_config_path(pytestconfig):
    return pytestconfig.option.service_config


@pytest.fixture(scope='session')
def service_config_vars_path(pytestconfig):
    return pytestconfig.option.service_config_vars


@pytest.fixture(scope='session')
def config_fallback_path(pytestconfig):
    return pytestconfig.option.config_fallback


@pytest.fixture(scope='session')
def service_tmpdir(service_binary, tmp_path_factory):
    return tmp_path_factory.mktemp(pathlib.Path(service_binary).name)


@pytest.fixture(scope='session')
def service_config_path_temp(
        service_tmpdir, service_config_yaml,
) -> pathlib.Path:
    dst_path = service_tmpdir / 'config.yaml'
    dst_path.write_text(yaml.dump(service_config_yaml))
    return dst_path


@pytest.fixture(scope='session')
def service_config_yaml(_service_config):
    return _service_config.config_yaml


@pytest.fixture(scope='session')
def service_config_vars(_service_config):
    return _service_config.config_vars


@pytest.fixture(scope='session')
def _service_config(
        pytestconfig,
        request,
        service_tmpdir,
        service_config_path,
        service_config_vars_path,
) -> UserverConfig:
    config_vars: dict
    config_yaml: dict

    with open(service_config_path, mode='rt') as fp:
        config_yaml = yaml.safe_load(fp)

    if service_config_vars_path:
        with open(service_config_vars_path, mode='rt') as fp:
            config_vars = yaml.safe_load(fp)
    else:
        config_vars = {}

    plugin = pytestconfig.pluginmanager.get_plugin('userver_config')
    for hook in plugin.userver_config_hooks:
        if not callable(hook):
            hook_func = request.getfixturevalue(hook)
        else:
            hook_func = hook
        hook_func(config_yaml, config_vars)

    if not config_vars:
        config_yaml.pop('config_vars', None)
    else:
        config_vars_path = service_tmpdir / 'config_vars.yaml'
        config_vars_path.write_text(yaml.dump(config_vars))
        config_yaml['config_vars'] = str(config_vars_path)

    return UserverConfig(config_yaml=config_yaml, config_vars=config_vars)


@pytest.fixture(scope='session')
def userver_config_base(service_port, monitor_port):
    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        if 'server' in components:
            server = components['server']
            if 'listener' in server:
                server['listener']['port'] = service_port

            if 'listener-monitor' in server:
                server['listener-monitor']['port'] = monitor_port

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
        config_vars['testsuite-enabled'] = True
        components = config_yaml['components_manager']['components']
        if 'tests-control' in components:
            components['tests-control']['testpoint-url'] = mockserver_info.url(
                'testpoint',
            )

    return _patch_config


@pytest.fixture(scope='session')
def userver_config_fallback(pytestconfig, config_fallback_path):
    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        for component_name in (
                'taxi-config-fallbacks',
                'taxi-config-client-updater',
                'dynamic-config-fallbacks',
                'dynamic-config-client-updater',
        ):
            if component_name not in components:
                continue
            component = components[component_name]
            if not config_fallback_path:
                pytest.fail('Please run with --config-fallback=...')
            component['fallback-path'] = str(config_fallback_path)

    return _patch_config


class TaxiConfig:
    """Simple config backend."""

    def __init__(self):
        self._values = {}

    def set_values(self, values):
        self._values.update(values)

    def get_values(self):
        return self._values.copy()

    def remove_values(self, keys):
        for key in keys:
            if key not in self._values:
                raise ConfigNotFoundError(
                    f'param "{key}" is not found in config',
                )
            self._values.pop(key)

    def set(self, **values):
        self.set_values(values)

    def get(self, key, default=None):
        if key not in self._values:
            if default is not None:
                return default
            raise ConfigNotFoundError(
                'param "%s" is not found in config' % (key,),
            )
        return self._values[key]

    def remove(self, key):
        return self.remove_values([key])


@pytest.fixture
def taxi_config(
        request,
        search_path,
        load_json,
        object_substitute,
        config_service_defaults,
):
    config = TaxiConfig()
    all_values = config_service_defaults.copy()
    for path in reversed(list(search_path('config.json'))):
        values = load_json(path)
        all_values.update(values)
    for marker in request.node.iter_markers('config'):
        marker_json = object_substitute(marker.kwargs)
        all_values.update(marker_json)
    config.set_values(all_values)
    return config


@pytest.fixture(scope='session')
def config_service_defaults():
    """
    Fixture that returns default values for config. You have to override
    it in your local conftest.py or fixture:

    @pytest.fixture(scope='session')
    def config_service_defaults():
        with open('defaults.json') as fp:
            return json.load(fp)
    """
    raise RuntimeError(
        'In order to use fixture %s you have to override %s fixture'
        % ('mock_configs_service', config_service_defaults.__name__),
    )
