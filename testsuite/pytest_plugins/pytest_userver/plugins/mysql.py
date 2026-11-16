"""
Plugin that starts MySQL/MariaDB and redirects the default local secdist endpoint
to the database selected by testsuite.
"""

import json

import pytest

import pytest_userver.config

pytest_plugins = [
    'testsuite.databases.mysql.pytest_plugin',
    'pytest_userver.plugins.core',
]

_DEFAULT_MYSQL_PORT = 13307
_LOCAL_MYSQL_HOSTS = (['localhost'], ['127.0.0.1'], ['::1'])


@pytest.fixture(scope='session')
@pytest_userver.config.patch
def userver_config_mysql(mysql_conninfo, service_env):
    """
    Redirect the default local MySQL endpoint in environment-backed secdist
    to `mysql_conninfo`. Preserve explicit endpoints, including chaos proxies.

    Only single loopback hosts on port 13307 are remapped. Override this fixture
    to customize or disable the patch. File-backed secdist is not modified.

    @ingroup userver_testsuite_fixtures
    """

    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        provider = components.get('default-secdist-provider') or {}
        env_key = provider.get('environment-secrets-key')
        if not env_key or not service_env or env_key not in service_env:
            return

        secdist = json.loads(service_env[env_key])
        if _patch_mysql_secdist(secdist, mysql_conninfo):
            service_env[env_key] = json.dumps(secdist)

    return _patch_config


def _patch_mysql_secdist(secdist, conninfo):
    changed = False
    for settings in secdist.get('mysql_settings', {}).values():
        if settings.get('hosts') not in _LOCAL_MYSQL_HOSTS or settings.get('port') != _DEFAULT_MYSQL_PORT:
            continue
        settings.update(
            hosts=[conninfo.hostname],
            port=conninfo.port,
            user=conninfo.user,
            password=conninfo.password or '',
        )
        changed = True
    return changed
