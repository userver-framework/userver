import pytest
from http import HTTPStatus

from endpoints import register_user, update_user
from models import User
from validators import validate_user
from utils import get_user_token


async def test_update_user(service_client):
    user = User()
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    another_user = User()
    response = await update_user(service_client, another_user, get_user_token(response))
    assert response.status == HTTPStatus.OK
    assert validate_user(another_user, response)


async def test_update_user_unauthorized(service_client):
    user = User()
    response = await update_user(service_client, user, 'some-token')
    assert response.status == HTTPStatus.UNAUTHORIZED
