import pytest
from http import HTTPStatus

from endpoints import register_user, get_user
from models import User
from validators import validate_user
from utils import get_user_token


async def test_get_user(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    response = await get_user(service_client, get_user_token(response))
    assert response.status == HTTPStatus.OK
    assert validate_user(user, response)


async def test_get_user_unauthorized(service_client):
    response = await get_user(service_client, None)
    assert response.status == HTTPStatus.UNAUTHORIZED

    response = await get_user(service_client, 'some-token')
    assert response.status == HTTPStatus.UNAUTHORIZED

    jwt_token = (
        'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.'
        'eyJpZCI6ImlkIn0.'
        'd9NfuI8JzGnYtjPa5rHRh4Jr104WKp-yls9POZJbe9U'
    )

    response = await get_user(service_client, 'Bearer {token}'.format(token=jwt_token))
    assert response.status == HTTPStatus.UNAUTHORIZED

    response = await get_user(service_client, 'Token {token}'.format(token=jwt_token))
    assert response.status == HTTPStatus.UNAUTHORIZED