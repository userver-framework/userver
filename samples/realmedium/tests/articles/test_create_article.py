import pytest
from http import HTTPStatus

from endpoints import register_user, create_article
from models import User, Profile, Article
from validators import validate_article
from utils import get_user_token
from testsuite.utils import matching


async def test_create_article_unauthorized(service_client):
    user = User(bio=None, image=None)
    article = Article(Profile(user))
    response = await create_article(service_client, article, None)
    assert response.status == HTTPStatus.UNAUTHORIZED


async def test_create_article(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    article = Article(Profile(user))
    response = await create_article(service_client, article, get_user_token(response))
    assert response.status == HTTPStatus.OK
    assert validate_article(article, response)
