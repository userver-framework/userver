import pytest
from testsuite.utils import compat


@pytest.fixture
def service_log_capture(userver_log_capture, tests_control):
    @compat.asynccontextmanager
    async def do_capture():
        async with userver_log_capture.start_capture() as capture:
            await tests_control({'socket_logging_duplication': True})
            try:
                yield capture
            finally:
                await tests_control({'socket_logging_duplication': False})

    return do_capture


async def test_select(test_service_client, service_log_capture):
    async with service_log_capture() as capture:
        response = await test_service_client.get('/logcapture')
        assert response.status == 200

    records = capture.select(
        text='Message to catpure', link=response.headers['x-yarequestid'],
    )
    assert len(records) == 1


async def test_subscribe(test_service_client, service_log_capture, mockserver):
    async with service_log_capture() as capture:

        @capture.subscribe(
            text='Message to catpure', trace_id=mockserver.trace_id,
        )
        def log_event(link, **other):
            pass

        response = await test_service_client.get(
            '/logcapture', headers={'x-yatraceid': mockserver.trace_id},
        )
        assert response.status == 200

        call = await log_event.wait_call()
        assert call['link'] == response.headers['x-yarequestid']
