import pytest
from http import HTTPStatus

from endpoints import register_user
from models import User
from validators import validate_user

async def test_register(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK
    assert validate_user(user, response)


async def test_register_same_username(service_client):
    user = User()
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    another_user = User(username=user.username)
    response = await register_user(service_client, another_user)
    assert response.status == HTTPStatus.UNPROCESSABLE_ENTITY
    assert 'errors' in response.json()


async def test_register_same_email(service_client):
    user = User()
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    another_user = User(email=user.email)
    response = await register_user(service_client, another_user)
    assert response.status == HTTPStatus.UNPROCESSABLE_ENTITY
    assert 'errors' in response.json()


async def test_register_same_password(service_client):
    user = User()
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    another_user = User(password=user.password)
    response = await register_user(service_client, another_user)
    assert response.status == HTTPStatus.OK