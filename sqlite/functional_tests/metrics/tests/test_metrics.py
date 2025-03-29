import re
import pytest


def _normalize_metrics(metrics: str) -> str:
    result = []
    for line in metrics.splitlines():
        line = line.strip()
        if line.startswith('#') or not line:
            continue

        left, _ = line.rsplit('\t', 1)
        result.append(
            re.sub('localhost_\\d+', 'localhost_00000', left + '\t' + '0'),
        )
    result.sort()
    return '\n'.join(result)

@pytest.mark.parametrize("sqlite_db", [{"db_path": "tmp_kv.db"}], indirect=True)
async def test_metrics_smoke(sqlite_db, monitor_client):
    metrics = await monitor_client.metrics(prefix='sqlite.')
    assert len(metrics) > 1

@pytest.mark.parametrize("sqlite_db", [{"db_path": "tmp_kv.db"}], indirect=True)
async def test_metrics_portability(sqlite_db, service_client):
    warnings = await service_client.metrics_portability(prefix='sqlite.')
    assert not warnings


@pytest.mark.parametrize("sqlite_db", [{"db_path": "tmp_kv.db"}], indirect=True)
async def test_metrics(sqlite_db, service_client, monitor_client, load):
    # Forcing statement timing metrics to appear
    response = await service_client.get('/metrics/sqlite?key=test_key')
    assert response.status == 200
    assert response.content == b''

    response = await service_client.post(
        '/metrics/sqlite?key=test_key&value=test_value',
    )
    assert response.status == 200
    assert response.content == b'test_key: test_value'

    response = await service_client.get('/metrics/sqlite?key=test_key')
    assert response.status == 200
    assert response.content == b'test_key: test_value'

    response = await service_client.delete('/metrics/sqlite?key=test_key')
    assert response.status == 200
    assert response.content == b'Deleted by key: test_key'

    ethalon = _normalize_metrics(load('metrics_values.txt'))
    # assert ethalon
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(
            output_format='pretty', prefix='sqlite.',
        ),
    )
    assert all_metrics == ethalon
