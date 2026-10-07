HANDLER_PATH = '/chaos/httpserver'
MULTIPART_CONTENT_TYPE = 'multipart/form-data; boundary=zzz'
MALFORMED_BODY = 'this is not a multipart/form-data body'


async def test_malformed_multipart_body_reaches_handler(service_client):
    response = await service_client.post(
        HANDLER_PATH,
        params={'type': 'echo'},
        data=MALFORMED_BODY,
        headers={'Content-Type': MULTIPART_CONTENT_TYPE},
    )

    assert response.status_code == 200
    assert response.text == MALFORMED_BODY


async def test_empty_multipart_body_reaches_handler(service_client):
    response = await service_client.post(
        HANDLER_PATH,
        params={'type': 'echo'},
        data='',
        headers={'Content-Type': MULTIPART_CONTENT_TYPE},
    )

    assert response.status_code == 200
    assert response.text == ''
