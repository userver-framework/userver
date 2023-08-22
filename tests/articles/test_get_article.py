import pytest
import time
from http import HTTPStatus

from endpoints import register_user, create_article, get_article
from models import User, Profile, Article
from validators import validate_article
from utils import get_user_token


async def test_get_article(service_client):
    user = User(bio=None, image=None)

    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user_token = get_user_token(response)

    article = Article(Profile(user))
    response = await create_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK

    await service_client.invalidate_caches()
    response = await get_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK
    assert validate_article(article, response)


async def test_get_unknown_article(service_client):
    user = User(bio=None, image=None)

    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user_token = get_user_token(response)

    article = Article(Profile(user))
    await service_client.invalidate_caches()
    response = await get_article(service_client, article, user_token)
    assert response.status == HTTPStatus.NOT_FOUND


async def test_get_article_unauthorized(service_client):
    user = User(bio=None, image=None)

    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user_token = get_user_token(response)

    article = Article(Profile(user))
    response = await create_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK

    await service_client.invalidate_caches()
    response = await get_article(service_client, article, None)
    assert response.status == HTTPStatus.OK
    assert validate_article(article, response)
