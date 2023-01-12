"""
Auto adjusting of PostgreSQL component configs for the testsuite.
"""

import pytest


USERVER_CONFIG_HOOKS = ['_userver_pg_config']


@pytest.fixture(scope='session')
def _userver_pg_config(pgsql_local):
    if len(pgsql_local) != 1:
        raise ValueError(
            f'Found more than one entry in pgsql_local: '
            f'{list(pgsql_local.keys())}. '
            f'The "pytest_userver.plugins.pg_conf" pytest plugin supports '
            f'only one entry in pgsql_local fixture. The plugin should '
            f'be removed from pytest_plugins and '
            f'the "dbconnection" for the components::Postgres '
            f'components should be adjusted via config_vars.yaml or '
            f'USERVER_CONFIG_HOOKS.',
        )

    uri = list(pgsql_local.values())[0].get_uri()

    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        postgre_dbs = {
            name: params
            for name, params in components.items()
            if params and 'dbconnection' in params
        }

        if not postgre_dbs:
            raise ValueError(
                'Found no components with "dbconnection". '
                'The "pytest_userver.plugins.pg_conf" pytest plugin should '
                'be either removed or a components::Postgres component '
                'configuration is missing in static config.',
            )

        if len(postgre_dbs) > 1:
            raise ValueError(
                f'Found more than one components with "dbconnection": '
                f'{list(postgre_dbs.keys())}. '
                f'The "pytest_userver.plugins.pg_conf" pytest plugin should '
                f'be removed from pytest_plugins and '
                f'the "dbconnection" for the components::Postgres '
                f'components should be adjusted via config_vars.yaml or '
                f'USERVER_CONFIG_HOOKS.',
            )

        for config in postgre_dbs.values():
            config['dbconnection'] = uri

    return _patch_config
