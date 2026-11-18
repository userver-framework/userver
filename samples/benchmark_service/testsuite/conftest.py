import pathlib

import pytest
import pytest_userver.config

from testsuite.databases.pgsql import discover

pytest_plugins = [
    'pytest_userver.plugins.core',
    'pytest_userver.plugins.postgresql',
]


@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    databases = discover.find_schemas(
        'admin',
        [service_source_dir.joinpath('schemas/postgresql')],
    )
    return pgsql_local_create(list(databases.values()))


@pytest.fixture(scope='session')
@pytest_userver.config.patch
def benchmark_config_hook(service_source_dir):
    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['fs-cache-static']['dir'] = str(
            pathlib.Path(service_source_dir).joinpath('data/static'),
        )
        components['dataset-provider']['dataset-path'] = str(
            pathlib.Path(service_source_dir).joinpath('data/dataset.json'),
        )
        listener = components['server']['listener']
        for port_cfg in listener.get('ports', []):
            tls = port_cfg.get('tls')
            if not tls:
                continue
            tls['cert'] = str(pathlib.Path(service_source_dir).joinpath(tls['cert']))
            tls['private-key'] = str(
                pathlib.Path(service_source_dir).joinpath(tls['private-key']),
            )

    return _patch_config
