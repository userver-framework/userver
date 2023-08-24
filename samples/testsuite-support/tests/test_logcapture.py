# /// [select]
async def test_select(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.get('/logcapture')
        assert response.status == 200

    records = capture.select(
        text='Message to capture', link=response.headers['x-yarequestid'],
    )
    assert len(records) == 1, capture.select()
    # /// [select]


async def test_subscribe(service_client, mockserver):
    async with service_client.capture_logs() as capture:

        @capture.subscribe(
            text='Message to capture', trace_id=mockserver.trace_id,
        )
        def log_event(link, **other):
            pass

        response = await service_client.get(
            '/logcapture', headers={'x-yatraceid': mockserver.trace_id},
        )
        assert response.status == 200

        call = await log_event.wait_call()
        assert call['link'] == response.headers['x-yarequestid']
