import pytest
from http import HTTPStatus

from endpoints import register_user, create_article, add_comment
from models import User, Profile, Article, Comment
from validators import validate_comment
from utils import get_user_token


async def test_add_self_comment(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user_token = get_user_token(response)
    profile = Profile(user)
    article = Article(profile)
    response = await create_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK

    comment = Comment(profile)
    response = await add_comment(service_client, comment, article, user_token)
    assert response.status == HTTPStatus.OK
    assert validate_comment(comment, response)


async def test_add_not_self_comment(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK
    user_token = get_user_token(response)

    another_user = User(bio=None, image=None)
    response = await register_user(service_client, another_user)
    assert response.status == HTTPStatus.OK
    another_user_token = get_user_token(response)

    another_profile = Profile(another_user)
    article = Article(another_profile)
    response = await create_article(service_client, article, another_user_token)
    assert response.status == HTTPStatus.OK

    comment = Comment(Profile(user))
    response = await add_comment(service_client, comment, article, user_token)
    assert response.status == HTTPStatus.OK
    assert validate_comment(comment, response)


async def test_add_comment_unknown_article(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user_token = get_user_token(response)
    profile = Profile(user)
    article = Article(profile)
    comment = Comment(profile)
    response = await add_comment(service_client, comment, article, user_token)
    assert response.status == HTTPStatus.NOT_FOUND


async def test_add_comment_unauthorized(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user_token = get_user_token(response)
    profile = Profile(user)
    article = Article(profile)
    response = await create_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK

    comment = Comment(profile)
    response = await add_comment(service_client, comment, article, None)
    assert response.status == HTTPStatus.UNAUTHORIZED
