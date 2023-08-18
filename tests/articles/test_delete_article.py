import pytest
from http import HTTPStatus

from endpoints import register_user, create_article, delete_article, get_article
from models import User, Profile, Article
from utils import get_user_token


async def test_delete_article_unauthorized(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    article = Article(Profile(user))
    response = await create_article(service_client, article, get_user_token(response))
    assert response.status == HTTPStatus.OK

    response = await delete_article(service_client, article, None)
    assert response.status == HTTPStatus.UNAUTHORIZED


async def test_delete_unknown_article(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    article = Article(Profile(user))
    response = await delete_article(service_client, article, get_user_token(response))
    assert response.status == HTTPStatus.NOT_FOUND


async def test_delete_article(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user_token = get_user_token(response)

    article = Article(Profile(user))
    response = await create_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK

    response = await delete_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK

    response = await get_article(service_client, article, user_token)
    assert response.status == HTTPStatus.NOT_FOUND


async def test_invalid_access_delete_article(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    article = Article(Profile(user))
    response = await create_article(service_client, article, get_user_token(response))
    assert response.status == HTTPStatus.OK

    another_user = User(bio=None, image=None)
    response = await register_user(service_client, another_user)
    assert response.status == HTTPStatus.OK

    response = await delete_article(service_client, article, get_user_token(response))
    assert response.status == HTTPStatus.NOT_FOUND
