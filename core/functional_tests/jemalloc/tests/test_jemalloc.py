import glob
import re

import pytest

SYMBOL_LINE_RE = re.compile(r'0x[0-9a-f]+ .+')


async def test_heap_dump_is_served_and_symbolized(monitor_client):
    response = await monitor_client.post('service/jemalloc/pprof/enable')
    if response.status == 501:
        pytest.skip('userver was built without jemalloc support')

    assert response.status == 200, response.text
    try:
        response = await monitor_client.post('service/jemalloc/pprof/stat')
        assert response.status == 200
        assert response.text

        temp_files_before = set(glob.glob('/tmp/jeprof*'))

        response = await monitor_client.get('service/jemalloc/pprof/heap')

        assert response.status == 200
        dump = response.content
        assert dump.startswith(b'heap_v2/'), dump[:64]

        assert set(glob.glob('/tmp/jeprof*')) == temp_files_before

        addresses = _heap_dump_addresses(dump)
        assert addresses, dump[:512]

        response = await monitor_client.post('service/jemalloc/pprof/symbol', data='+'.join(sorted(addresses)))
        assert response.status == 200
        symbolized = _symbolized_names(response.text)

        assert any('utils::jemalloc::Stats' in name for name in symbolized.values()), (
            f'utils::jemalloc::Stats is missing from symbolized heap dump; '
            f'first symbols: {sorted(symbolized.values())[:20]}'
        )
    finally:
        response = await monitor_client.post('service/jemalloc/pprof/disable')
        assert response.status == 200


def _heap_dump_addresses(dump: bytes) -> set[str]:
    addresses = set()
    for line in dump.decode('latin-1').split('\n'):
        if line.startswith('@ '):
            addresses.update(line.split()[1:])
    return addresses


def _symbolized_names(response_text: str) -> dict[str, str]:
    names = {}
    for line in response_text.split('\n'):
        if not line:
            continue
        assert SYMBOL_LINE_RE.fullmatch(line), line
        address, name = line.split(' ', 1)
        names[address] = name
    return names
