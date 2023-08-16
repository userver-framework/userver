import pytest
from http import HTTPStatus

from endpoints import register_user, get_profile
from models import User
from validators import validate_profile
from utils import get_user_token


async def test_get_profile_auth(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    response = await get_profile(service_client, user, get_user_token(response))
    assert response.status == HTTPStatus.OK
    assert validate_profile(user, False, response)


async def test_get_profile_unauthorized(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    response = await get_profile(service_client, user, None)
    assert response.status == HTTPStatus.OK
    assert validate_profile(user, False, response)
