import pytest
from http import HTTPStatus

from endpoints import register_user, login_user
from models import User
from validators import validate_user


async def test_login(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    response = await login_user(service_client, user)
    assert response.status == HTTPStatus.OK
    assert validate_user(user, response)


async def test_login_unknown(service_client):
    user = User(bio=None, image=None)
    response = await login_user(service_client, user)
    assert response.status == HTTPStatus.NOT_FOUND