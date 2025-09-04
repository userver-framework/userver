import pytest

pytest_plugins = ['pytest_userver.plugins.core']

USERVER_CONFIG_HOOKS = ['userver_sqlite_config']


@pytest.fixture(scope='session')
def userver_sqlite_config(tmp_path_factory):
    """
    Returns a function that adjusts the SQLite static configuration file for the testsuite.
    Sets the `db-path` to be an in memory DB with shared cache.

    @ingroup userver_testsuite_fixtures
    """

    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        for _, params in components.items():
            if params and 'db-path' in params:
                params['db-path'] = f'file:{params["db-path"]}?mode=memory&cache=shared'

    return _patch_config


@pytest.fixture(scope='session')
def _list_dbpath_components(service_config_yaml):
    components_list = list()
    components = service_config_yaml['components_manager']['components']
    for commponent_name, params in components.items():
        if params and 'db-path' in params:
            components_list.append(commponent_name)
    return components_list


@pytest.fixture(autouse=True)
async def sqlite_db(_list_dbpath_components, service_client, testpoint):
    """
    Removes all the data from SQLite tables at the end of the test to guarantee tests isolation.

    @ingroup userver_testsuite_fixtures
    """
    yield

    for component in _list_dbpath_components:
        try:
            await service_client.run_task(f'sqlite/{component}/clean-db')
        except service_client.TestsuiteTaskNotFound:
            pass
