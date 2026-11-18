# /// [Functional test]
import json

import aiohttp


async def test_ok(service_client, load_binary):
    form_data = aiohttp.FormData()

    # Uploading file
    image = load_binary('logo_in_circle.png')
    form_data.add_field('profileImage', image, filename='logo_in_circle.png')

    # Adding JSON payload
    address = {'street': '3, Garden St', 'city': 'Hillsbery, UT'}
    form_data.add_field(
        'address',
        json.dumps(address),
        content_type='application/json',
    )

    # Making a request and checking the result
    response = await service_client.post('/v1/multipart', data=form_data)
    assert response.status == 200
    assert 'application/json' in response.headers['Content-Type']
    assert response.text == f'city={address["city"]} image_size={len(image)}'
    # /// [Functional test]


async def test_bad_content_type(service_client):
    response = await service_client.post('/v1/multipart', data='{}')
    assert response.status == 400
    assert response.content == b"Expected 'multipart/form-data' content type"


MULTIPART_CONTENT_TYPE = 'multipart/form-data; boundary=zzz'
PNG_MAGIC_BYTES = b'\x89PNG\r\n\x1a\n'


async def test_malformed_body_is_rejected_by_the_handler(service_client):
    response = await service_client.post(
        '/v1/multipart',
        data=b'this is not a multipart/form-data body',
        headers={'Content-Type': MULTIPART_CONTENT_TYPE},
    )

    assert response.status == 400
    assert response.content == b'Expecting PNG image format'


async def test_partially_parsed_body_exposes_no_args(service_client):
    # The parser fills in 'profileImage' from the first part and only then fails
    # on the truncated second one, so this pins that a partially parsed form is
    # never handed to the handler.
    body = (
        b'--zzz\r\n'
        b'Content-Disposition: form-data; name="profileImage"; filename="x.png"\r\n'
        b'\r\n' + PNG_MAGIC_BYTES + b'\r\n'
        b'--zzz\r\n'
        b'Content-Disposition: form-data; name="address"\r\n'
    )

    response = await service_client.post(
        '/v1/multipart',
        data=body,
        headers={'Content-Type': MULTIPART_CONTENT_TYPE},
    )

    assert response.status == 400
    assert response.content == b'Expecting PNG image format'
