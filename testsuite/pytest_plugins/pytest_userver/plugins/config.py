"""
Work with the configuration files of the service in testsuite.
"""

# pylint: disable=redefined-outer-name
import copy
import logging
import os
import pathlib
import types
import typing

import pytest
import yaml


# flake8: noqa E266
## Fixtures and functions in USERVER_CONFIG_HOOKS used to change the
## static config or config_vars.yaml.
##
## Functions and fixtures that are listed in the USERVER_CONFIG_HOOKS variable
## in your pytest-plugin are run before config is written to disk, so that the
## service, the pytest_userver.plugins.config.service_config_yaml and
## the pytest_userver.plugins.config.service_config_vars get the modified
## values.
##
## Example of patching config :
##
## @snippet samples/grpc_service/tests/conftest.py Prepare configs
##
## @hideinitializer
USERVER_CONFIG_HOOKS = [
    'userver_config_http_server',
    'userver_config_http_client',
    'userver_config_logging',
    'userver_config_testsuite',
    'userver_config_secdist',
    'userver_config_testsuite_middleware',
]


# @cond


logger = logging.getLogger(__name__)


class _UserverConfigPlugin:
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


class _UserverConfig(typing.NamedTuple):
    config_yaml: dict
    config_vars: dict


def pytest_configure(config):
    config.pluginmanager.register(_UserverConfigPlugin(), 'userver_config')
    config.addinivalue_line(
        'markers', 'config: per-test dynamic config values',
    )


def pytest_addoption(parser) -> None:
    group = parser.getgroup('userver-config')
    group.addoption(
        '--service-log-level',
        type=str.lower,
        choices=['trace', 'debug', 'info', 'warning', 'error', 'critical'],
    )
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
        '--service-secdist',
        type=pathlib.Path,
        help='Path to secure_data.json file.',
    )
    group.addoption(
        '--config-fallback',
        type=pathlib.Path,
        help='Path to dynamic config fallback file.',
    )


# @endcond


@pytest.fixture(scope='session')
def service_config_path(pytestconfig) -> pathlib.Path:
    """
    Returns the path to service.yaml file set by command line
    `--service-config` option.

    Override this fixture to change the way path to service.yaml is provided.

    @ingroup userver_testsuite_fixtures
    """
    return pytestconfig.option.service_config


@pytest.fixture(scope='session')
def service_config_vars_path(pytestconfig) -> typing.Optional[pathlib.Path]:
    """
    Returns the path to config_vars.yaml file set by command line
    `--service-config-vars` option.

    Override this fixture to change the way path to config_vars.yaml is
    provided.

    @ingroup userver_testsuite_fixtures
    """
    return pytestconfig.option.service_config_vars


@pytest.fixture(scope='session')
def service_secdist_path(pytestconfig) -> typing.Optional[pathlib.Path]:
    """
    Returns the path to secure_data.json file set by command line
    `--service-secdist` option.

    Override this fixture to change the way path to secure_data.json is
    provided.

    @ingroup userver_testsuite_fixtures
    """
    return pytestconfig.option.service_secdist


@pytest.fixture(scope='session')
def config_fallback_path(pytestconfig) -> pathlib.Path:
    """
    Returns the path to dynamic config fallback file set by command line
    `--config-fallback` option.

    Override this fixture to change the way path to dynamic config fallback is
    provided.

    @ingroup userver_testsuite_fixtures
    """
    return pytestconfig.option.config_fallback


@pytest.fixture(scope='session')
def service_tmpdir(service_binary, tmp_path_factory):
    """
    Returns the path for temporary files. The path is the same for the whole
    session and files are not removed (at least by this fixture) between
    tests.

    @ingroup userver_testsuite_fixtures
    """
    return tmp_path_factory.mktemp(pathlib.Path(service_binary).name)


@pytest.fixture(scope='session')
def service_config_path_temp(
        service_tmpdir, service_config, service_config_yaml,
) -> pathlib.Path:
    """
    Dumps the contents of the service_config_yaml into a static config for
    testsuite and returns the path to the config file.

    @ingroup userver_testsuite_fixtures
    """
    dst_path = service_tmpdir / 'config.yaml'

    logger.debug(
        'userver fixture "service_config_path_temp" writes the patched static '
        'config to "%s" equivalent to:\n%s',
        dst_path,
        yaml.dump(service_config),
    )
    dst_path.write_text(yaml.dump(service_config_yaml))

    return dst_path


@pytest.fixture(scope='session')
def service_config_yaml(_service_config_hooked) -> dict:
    """
    Returns the static config values after the USERVER_CONFIG_HOOKS were
    applied (if any). Prefer using
    pytest_userver.plugins.config.service_config

    @ingroup userver_testsuite_fixtures
    """
    return _service_config_hooked.config_yaml


@pytest.fixture(scope='session')
def service_config_vars(_service_config_hooked) -> dict:
    """
    Returns the static config variables (config_vars.yaml) values after the
    USERVER_CONFIG_HOOKS were applied (if any). Prefer using
    pytest_userver.plugins.config.service_config

    @ingroup userver_testsuite_fixtures
    """
    return _service_config_hooked.config_vars


def _substitute_values(config, service_config_vars: dict, service_env) -> None:
    if isinstance(config, dict):
        for key, value in config.items():
            if not isinstance(value, str):
                _substitute_values(value, service_config_vars, service_env)
                continue

            if not value.startswith('$'):
                continue

            new_value = service_config_vars.get(value[1:])
            if new_value is not None:
                config[key] = new_value
                continue

            env = config.get(f'{key}#env')
            if env:
                if service_env:
                    new_value = service_env.get(service_env)
                if not new_value:
                    new_value = os.environ.get(env)
                if new_value:
                    config[key] = new_value
                    continue

            fallback = config.get(f'{key}#fallback')
            if fallback:
                config[key] = fallback

    if isinstance(config, list):
        for i, value in enumerate(config):
            if not isinstance(value, str):
                _substitute_values(value, service_config_vars, service_env)
                continue

            if not value.startswith('$'):
                continue

            new_value = service_config_vars.get(value[1:])
            if new_value is not None:
                config[i] = new_value


@pytest.fixture(scope='session')
def service_config(
        service_config_yaml, service_config_vars, service_env,
) -> dict:
    """
    Returns the static config values after the USERVER_CONFIG_HOOKS were
    applied (if any) and with all the '$', environment and fallback variables
    substituted.

    @ingroup userver_testsuite_fixtures
    """
    config = copy.deepcopy(service_config_yaml)
    _substitute_values(config, service_config_vars, service_env)
    config.pop('config_vars', None)
    return config


@pytest.fixture(scope='session')
def _original_service_config(
        service_config_path, service_config_vars_path,
) -> _UserverConfig:
    config_vars: dict
    config_yaml: dict

    with open(service_config_path, mode='rt') as fp:
        config_yaml = yaml.safe_load(fp)

    if service_config_vars_path:
        with open(service_config_vars_path, mode='rt') as fp:
            config_vars = yaml.safe_load(fp)
    else:
        config_vars = {}

    return _UserverConfig(config_yaml=config_yaml, config_vars=config_vars)


@pytest.fixture(scope='session')
def _service_config_hooked(
        pytestconfig, request, service_tmpdir, _original_service_config,
) -> _UserverConfig:
    config_yaml = copy.deepcopy(_original_service_config.config_yaml)
    config_vars = copy.deepcopy(_original_service_config.config_vars)

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
        config_vars_text = yaml.dump(config_vars)
        logger.debug(
            'userver fixture "service_config" writes the patched static '
            'config vars to "%s":\n%s',
            config_vars_path,
            config_vars_text,
        )
        config_vars_path.write_text(config_vars_text)
        config_yaml['config_vars'] = str(config_vars_path)

    return _UserverConfig(config_yaml=config_yaml, config_vars=config_vars)


@pytest.fixture(scope='session')
def userver_config_http_server(service_port, monitor_port):
    """
    Returns a function that adjusts the static configuration file for testsuite.
    Sets the `server.listener.port` to listen on
    @ref pytest_userver.plugins.base.service_port "service_port" fixture value;
    sets the `server.listener-monitor.port` to listen on
    @ref pytest_userver.plugins.base.monitor_port "monitor_port"
    fixture value.

    @ingroup userver_testsuite_fixtures
    """

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
def allowed_url_prefixes_extra() -> typing.List[str]:
    """
    By default, userver HTTP client is only allowed to talk to mockserver
    when running in testsuite. This makes tests repeatable and encapsulated.

    Override this fixture to whitelist some additional URLs.
    It is still strongly advised to only talk to localhost in tests.

    @ingroup userver_testsuite_fixtures
    """
    return []


@pytest.fixture(scope='session')
def userver_config_http_client(
        mockserver_info, mockserver_ssl_info, allowed_url_prefixes_extra,
):
    """
    Returns a function that adjusts the static configuration file for testsuite.
    Sets increased timeout and limits allowed URLs for `http-client` component.

    @ingroup userver_testsuite_fixtures
    """

    def patch_config(config, config_vars):
        components: dict = config['components_manager']['components']
        if not {'http-client', 'testsuite-support'}.issubset(
                components.keys(),
        ):
            return
        http_client = components['http-client'] or {}
        http_client['testsuite-enabled'] = True
        http_client['testsuite-timeout'] = '10s'

        allowed_urls = [mockserver_info.base_url]
        if mockserver_ssl_info:
            allowed_urls.append(mockserver_ssl_info.base_url)
        allowed_urls += allowed_url_prefixes_extra
        http_client['testsuite-allowed-url-prefixes'] = allowed_urls

    return patch_config


@pytest.fixture(scope='session')
def userver_default_log_level() -> str:
    """
    Default log level to use in userver if no caoomand line option was provided.

    Returns 'debug'.

    @ingroup userver_testsuite_fixtures
    """
    return 'debug'


@pytest.fixture(scope='session')
def userver_log_level(pytestconfig, userver_default_log_level) -> str:
    """
    Returns --service-log-level value if provided, otherwise returns
    userver_default_log_level() value from fixture.

    @ingroup userver_testsuite_fixtures
    """
    if pytestconfig.option.service_log_level:
        return pytestconfig.option.service_log_level
    return userver_default_log_level


@pytest.fixture(scope='session')
def userver_config_logging(userver_log_level):
    """
    Returns a function that adjusts the static configuration file for testsuite.
    Sets the `logging.loggers.default` to log to `@stderr` with level set
    from `--service-log-level` pytest configuration option.

    @ingroup userver_testsuite_fixtures
    """

    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        if 'logging' in components:
            components['logging']['loggers'] = {
                'default': {
                    'file_path': '@stderr',
                    'level': userver_log_level,
                    'overflow_behavior': 'discard',
                },
            }
        config_vars['logger_level'] = userver_log_level

    return _patch_config


@pytest.fixture(scope='session')
def userver_config_testsuite(pytestconfig, mockserver_info):
    """
    Returns a function that adjusts the static configuration file for testsuite.

    Sets up `testsuite-support` component, which:

    - increases timeouts for userver drivers
    - disables periodic cache updates
    - enables testsuite tasks

    Sets the `testsuite-enabled` in config_vars.yaml to `True`; sets the
    `tests-control.testpoint-url` to mockserver URL.

    @ingroup userver_testsuite_fixtures
    """

    def _set_postgresql_options(testsuite_support: dict) -> None:
        testsuite_support['testsuite-pg-execute-timeout'] = '35s'
        testsuite_support['testsuite-pg-statement-timeout'] = '30s'
        testsuite_support['testsuite-pg-readonly-master-expected'] = True

    def _set_redis_timeout(testsuite_support: dict) -> None:
        testsuite_support['testsuite-redis-timeout-connect'] = '40s'
        testsuite_support['testsuite-redis-timeout-single'] = '30s'
        testsuite_support['testsuite-redis-timeout-all'] = '30s'

    def _disable_cache_periodic_update(testsuite_support: dict) -> None:
        testsuite_support['testsuite-periodic-update-enabled'] = False

    def patch_config(config, config_vars) -> None:
        components: dict = config['components_manager']['components']
        if 'testsuite-support' not in components:
            return
        testsuite_support = components['testsuite-support'] or {}
        testsuite_support['testsuite-increased-timeout'] = '30s'
        _set_postgresql_options(testsuite_support)
        _set_redis_timeout(testsuite_support)
        service_runner = pytestconfig.getoption('--service-runner-mode', False)
        if not service_runner:
            _disable_cache_periodic_update(testsuite_support)
        testsuite_support['testsuite-tasks-enabled'] = not service_runner
        testsuite_support[
            'testsuite-periodic-dumps-enabled'
        ] = '$userver-dumps-periodic'
        components['testsuite-support'] = testsuite_support

        config_vars['testsuite-enabled'] = True
        if 'tests-control' in components:
            components['tests-control']['testpoint-url'] = mockserver_info.url(
                'testpoint',
            )

    return patch_config


@pytest.fixture(scope='session')
def userver_config_secdist(service_secdist_path):
    """
    Returns a function that adjusts the static configuration file for testsuite.
    Sets the `default-secdist-provider.config` to the value of
    @ref pytest_userver.plugins.config.service_secdist_path "service_secdist_path"
    fixture.

    @ingroup userver_testsuite_fixtures
    """

    def _patch_config(config_yaml, config_vars):
        if not service_secdist_path:
            return

        components = config_yaml['components_manager']['components']
        if 'default-secdist-provider' not in components:
            return

        if not service_secdist_path.is_file():
            raise ValueError(
                f'"{service_secdist_path}" is not a file. Provide a '
                f'"--service-secdist" pytest option or override the '
                f'"service_secdist_path" fixture.',
            )
        components['default-secdist-provider']['config'] = str(
            service_secdist_path,
        )

    return _patch_config


@pytest.fixture(scope='session')
def userver_config_testsuite_middleware(
        userver_testsuite_middleware_enabled: bool,
):
    def patch_config(config_yaml, config_vars):
        if not userver_testsuite_middleware_enabled:
            return

        components = config_yaml['components_manager']['components']
        if 'server' not in components:
            return

        pipeline_builder = components.setdefault(
            'default-server-middleware-pipeline-builder', {},
        )
        middlewares = pipeline_builder.setdefault('append', [])
        middlewares.append('testsuite-exceptions-handling-middleware')

    return patch_config


@pytest.fixture(scope='session')
def userver_testsuite_middleware_enabled() -> bool:
    """Enabled testsuite middleware."""
    return True
