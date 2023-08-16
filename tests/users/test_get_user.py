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
    assert validate_user(user, response.json())


async def test_get_unknown_user(service_client):
    response = await get_user(service_client, 'some-token')
    assert response.status == HTTPStatus.UNAUTHORIZED