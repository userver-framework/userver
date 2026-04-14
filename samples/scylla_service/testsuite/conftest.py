# /// [scylla setup]
import json

import pytest

pytest_plugins = ['pytest_userver.plugins.core']
# /// [scylla setup]


# /// [service_env]
SECDIST_CONFIG = {
    'scylla_settings': {
        'scylla_example': {
            'hosts': 'scylla',
        },
    },
}


@pytest.fixture(scope='session')
def service_env():
    return {'SECDIST_CONFIG': json.dumps(SECDIST_CONFIG)}
    # /// [service_env]


@pytest.fixture(autouse=True)
async def _scylla_schema(service_client):
    response = await service_client.post('/v1/schema/init')
    assert response.status == 200

    await service_client.post('/v1/kv/truncate')
    await service_client.post(
        '/v1/raw',
        json={'query': 'TRUNCATE examples.events', 'void': True},
    )
