import os

import pytest

from taxi_tests.daemons import service_daemon
from taxi_tests.utils import yaml_util


class Settings:
    INITIAL_DATA_PATH = [
        os.path.abspath(
            os.path.join(os.path.dirname(__file__), '../default_fixtures')
        ),
    ]
    DB_SETTINGS_YAML_PATH = os.path.abspath(
        os.path.join(
            os.path.dirname(__file__),
            os.path.pardir,
            os.path.pardir,
            'db_settings.yaml',
        )
    )
    MOCKSERVER_OPTS = {
        'hostname': '127.0.0.1',
        'port': 9999
    }

    REDIS_PORT = 16379

    MONGO_CONNECTIONS = {}
    STQ_COLLECTIONS = {}

    POSTGRESQL_CONNECTION = (
        'host=/tmp/testsuite-postgresql user=testsuite dbname=taximeter'
    )


def pytest_addoption(parser):
    parser.addoption(
        '--build-dir',
        required=True,
        help='Path to build directory.'
    )
    parser.addoption(
        '--testsuite-dir',
        default=os.path.normpath(
            os.path.join(os.path.dirname(__file__), '..')
        ),
        help='Path to testsuite directory.'
    )
    parser.addoption(
        '--userver-disable',
        default=False, action='store_true',
        help='Do not start fastcgi daemon.'
    )


@pytest.fixture(scope='session')
def build_dir(request):
    return request.config.getoption('--build-dir')


@pytest.fixture(scope='session')
def settings(build_dir, pytestconfig):
    settings = Settings()
    settings.TAXI_BUILD_DIR = build_dir
    settings.TESTSUITE_CONFIGS = os.path.join(
        build_dir, 'testsuite/configs'
    )

    mongo_connections = {}
    # TODO: It might be a good idea to use connections required by service
    db_settings = yaml_util.load_file(settings.DB_SETTINGS_YAML_PATH)
    for alias, collection_info in db_settings.items():
        connection_name = collection_info['settings']['connection']
        # TODO: port is hardcoded in mongo startup script
        mongo_connections[connection_name] = 'mongodb://localhost:27117/'
    settings.MONGO_CONNECTIONS = mongo_connections
    settings.userver = service_daemon.ServiceSettings()

    return settings
