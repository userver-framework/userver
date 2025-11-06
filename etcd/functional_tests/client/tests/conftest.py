import asyncio
import base64
import json

import aiohttp

import pytest

pytest_plugins = ['pytest_userver.plugins.core']


@pytest.fixture(scope='session')
def userver_config_http_client(
    mockserver_info,
    mockserver_ssl_info,
    allowed_url_prefixes_extra,
):
    def patch_config(config, config_vars):
        components: dict = config['components_manager']['components']

        http_client = components['http-client'] or {}
        http_client['testsuite-enabled'] = False

        etcd_client = components['etcd-client'] or {}
        etcd_client['endpoints'] = [mockserver_info.base_url[:-1]]

        allowed_urls = [mockserver_info.base_url]
        if mockserver_ssl_info:
            allowed_urls.append(mockserver_ssl_info.base_url)
        allowed_urls += allowed_url_prefixes_extra
        http_client['testsuite-allowed-url-prefixes'] = allowed_urls

    return patch_config


@pytest.fixture(name='etcd_mock')
def etcd_mock(mockserver):
    etcd_storage = {}

    @mockserver.json_handler('/v3/kv/put')
    async def mock(request):
        key = base64.b64decode(request.json['key'])
        value = base64.b64decode(request.json['value'])
        etcd_storage[key] = value
        return mockserver.make_response('OK!')

    @mockserver.json_handler('/v3/kv/range')
    async def mock(request):
        request_key = base64.b64decode(request.json['key'])
        if 'range_end' in request.json:
            request_range_end = base64.b64decode(request.json['range_end'])
        elif request_key in etcd_storage:
            return mockserver.make_response(json={
                'kvs': [{
                    'key': base64.b64encode(request_key).decode('utf-8'),
                    'value': base64.b64encode(etcd_storage[request_key]).decode('utf-8'),
                    'version': '2',
                }]
            })
        else:
            return mockserver.make_response(json={'kvs': []})

        values = []
        for key, value in etcd_storage.items():
            if key >= request_key and key < request_range_end:
                values.append({
                    'key': base64.b64encode(key).decode('utf-8'),
                    'value': base64.b64encode(value).decode('utf-8'),
                    'version': '2',
                })

        return mockserver.make_response(json={
            'kvs': values
        })

    @mockserver.handler('/v3/watch')
    async def mock(request):
        key = base64.b64decode(request.json['create_request']['key'])
        response = aiohttp.web.StreamResponse(
            status=200,
            headers={
                'Content-Type': 'application/json',
            }
        )
        await response.prepare(request._request)
        data = json.dumps({
            'result': {
                'events': [
                    {
                        'kv': {
                            'key': base64.b64encode(key).decode(),
                            'value': base64.b64encode(b'new_value').decode(),
                            'version': '2',
                        }
                    }
                ]
            }
        })
        await response.write(data.encode())
        await asyncio.sleep(0.01)

        return response
