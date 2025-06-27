import asyncio

from testsuite.utils import http


async def test_basic(service_binary, tmp_path, mockserver):
    request_count = 10
    url_path = '/some-path'

    urls_file = tmp_path / 'urls.txt'
    urls_file.write_text(mockserver.url(url_path))

    @mockserver.handler(url_path)
    def _handler(request: http.Request) -> http.Response:
        assert request.method == 'GET'
        return mockserver.make_response('OK')

    subprocess = await asyncio.create_subprocess_exec(
        service_binary, '--url-file', urls_file, '--count', str(request_count)
    )
    assert await subprocess.wait() == 0

    assert _handler.times_called == request_count
