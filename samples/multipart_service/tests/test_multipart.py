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
        'address', json.dumps(address), content_type='application/json',
    )

    # Making a request and checking the result
    response = await service_client.post('/v1/multipart', data=form_data)
    assert response.status == 200
    assert response.text == f'city={address["city"]} image_size={len(image)}'
    # /// [Functional test]


async def test_bad_content_type(service_client):
    response = await service_client.post('/v1/multipart', data='{}')
    assert response.status == 400
    assert response.content == b'Expected \'multipart/form-data\' content type'
