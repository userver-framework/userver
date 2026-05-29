# /// [scylla setup]
import os

import pytest

from pytest_userver.plugins.scylla import ConnectionInfo

pytest_plugins = ['pytest_userver.plugins.scylla']
# /// [scylla setup]


@pytest.fixture(scope='session')
def scylla_connection_info(pytestconfig) -> ConnectionInfo:
    host = (
        pytestconfig.option.scylla_host
        or os.environ.get('TESTSUITE_SCYLLA_HOST')
        or 'scylla'
    )
    port = (
        pytestconfig.option.scylla_port
        or int(os.environ.get('TESTSUITE_SCYLLA_PORT') or 9042)
    )


    return ConnectionInfo(host=host, port=port)


@pytest.fixture(autouse=True)
async def _scylla_schema(service_client):
    response = await service_client.post('/v1/schema/init')
    assert response.status == 200

    await service_client.post('/v1/kv/truncate')
    await service_client.post(
        '/v1/raw',
        json={'query': 'TRUNCATE examples.events', 'void': True},
    )
