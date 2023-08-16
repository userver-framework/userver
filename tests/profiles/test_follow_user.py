import pytest
from http import HTTPStatus

from endpoints import register_user, get_profile, follow_user
from models import User
from validators import validate_profile
from utils import get_user_token

async def test_follow_user(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user_token = get_user_token(response)

    followed_user = User(bio=None, image=None)
    response = await register_user(service_client, followed_user)
    assert response.status == HTTPStatus.OK

    response = await follow_user(service_client, followed_user, user_token)
    assert response.status == HTTPStatus.OK
    assert validate_profile(followed_user, True, response)

    response = await get_profile(service_client, followed_user, user_token)
    assert response.status == HTTPStatus.OK
    assert validate_profile(followed_user, True, response)


async def test_follow_urself(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    response = await follow_user(service_client, user, get_user_token(response))
    assert response.status == HTTPStatus.BAD_REQUEST


async def test_follow_user_unauthorized(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    response = await follow_user(service_client, user, 'some-token')
    assert response.status == HTTPStatus.UNAUTHORIZED


async def test_follow_unknown_user(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user.username = 'some-username'
    response = await follow_user(service_client, user, get_user_token(response))
    assert response.status == HTTPStatus.NOT_FOUND