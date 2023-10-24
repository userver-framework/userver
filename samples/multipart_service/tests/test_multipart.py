# /// [Functional test]
import json
import pathlib

import aiohttp


async def test_ok(service_client):
    form_data = aiohttp.FormData()

    # Uploading file
    userver_root_dir = pathlib.Path(__file__).parent.parent.parent.parent
    image_path = userver_root_dir / 'scripts' / 'docs' / 'logo_in_circle.png'
    with open(image_path, mode='rb') as image_file:
        image = image_file.read()
        form_data.add_field('profileImage', image, filename=image_path.name)

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
