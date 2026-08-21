import re

NUM_SYMBOLS_RE = re.compile(r'num_symbols: [1-9][0-9]*')


async def test_monitor(monitor_client):
    response = await monitor_client.get('/service/log-level/')
    assert response.status == 200


async def test_metrics_smoke(monitor_client):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1


# /// [metrics partial portability]
async def test_partial_metrics_portability(service_client):
    warnings = await service_client.metrics_portability()
    warnings.pop('label_name_mismatch', None)
    assert not warnings, warnings
    # /// [metrics partial portability]


async def test_jemalloc_handle(monitor_client):
    response = await monitor_client.post('service/jemalloc/pprof/disable')
    if response.status_code == 501:
        # No jemalloc support
        return
    else:
        # Handle always succeeds even if was not previously enabled
        assert response.status_code == 200

    response = await monitor_client.post('service/jemalloc/pprof/stat')
    assert response.status_code == 200
    assert response.text

    try:
        response = await monitor_client.post('service/jemalloc/pprof/enable')
        assert response.status_code == 503
        assert 'MALLOC_CONF' in response.text
        assert 'prof:true' in response.text
    finally:
        response = await monitor_client.post('service/jemalloc/pprof/disable')
        assert response.status_code == 200


async def test_unknown_command_lists_the_supported_ones(monitor_client):
    response = await monitor_client.get('service/jemalloc/pprof/unknown')

    assert response.status == 404
    assert 'Unsupported command' in response.text
    for command in ('stat', 'enable', 'disable', 'dump', 'heap', 'symbol'):
        assert command in response.text, response.text


async def test_symbol_get_reports_that_symbolization_is_available(monitor_client):
    response = await monitor_client.get('service/jemalloc/pprof/symbol')

    assert response.status == 200
    first_line = response.text.split('\n')[0]
    assert NUM_SYMBOLS_RE.fullmatch(first_line), response.text


async def test_symbol_post_rejects_invalid_addresses(monitor_client):
    response = await monitor_client.post('service/jemalloc/pprof/symbol', data='0x0+not-an-address')

    assert response.status == 400
    assert 'invalid address' in response.text


async def test_symbol_post_rejects_too_many_addresses(monitor_client):
    body = '+'.join(hex(address) for address in range(1, 40000))

    response = await monitor_client.post('service/jemalloc/pprof/symbol', data=body)

    assert response.status == 413
    assert '32768' in response.text
