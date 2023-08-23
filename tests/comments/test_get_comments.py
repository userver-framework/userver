import pytest
from http import HTTPStatus

from endpoints import register_user, create_article, follow_user, add_comment, get_comments
from models import User, Profile, Article, Comment, CommentList
from validators import validate_comments
from utils import get_user_token


async def test_get_comments_authorized(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK
    user_token = get_user_token(response)
    profile = Profile(user)

    another_user = User(bio=None, image=None)
    response = await register_user(service_client, another_user)
    assert response.status == HTTPStatus.OK
    another_user_token = get_user_token(response)
    another_profile = Profile(another_user)

    response = await follow_user(service_client, another_user, user_token)
    assert response.status == HTTPStatus.OK
    another_profile.following = True

    article = Article(another_profile)
    response = await create_article(service_client, article, another_user_token)
    assert response.status == HTTPStatus.OK

    commentList = CommentList(5, another_profile)
    for comment in commentList.comments:
        response = await add_comment(service_client, comment, article, another_user_token)
        assert response.status == HTTPStatus.OK

    await service_client.invalidate_caches()
    response = await get_comments(service_client, article, user_token)
    assert response.status == HTTPStatus.OK
    assert validate_comments(commentList, response)


async def test_get_comments_unknown_article(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK
    user_token = get_user_token(response)
    profile = Profile(user)

    another_user = User(bio=None, image=None)
    response = await register_user(service_client, another_user)
    assert response.status == HTTPStatus.OK
    another_user_token = get_user_token(response)
    another_profile = Profile(another_user)

    article = Article(another_profile)
    response = await create_article(service_client, article, another_user_token)
    assert response.status == HTTPStatus.OK

    commentList = CommentList(5, another_profile)
    for comment in commentList.comments:
        response = await add_comment(service_client, comment, article, another_user_token)
        assert response.status == HTTPStatus.OK

    article.slug = 'some_slug'
    response = await get_comments(service_client, article, user_token)
    assert response.status == HTTPStatus.NOT_FOUND


async def test_get_comments_unauthorized(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK
    user_token = get_user_token(response)
    profile = Profile(user)

    another_user = User(bio=None, image=None)
    response = await register_user(service_client, another_user)
    assert response.status == HTTPStatus.OK
    another_user_token = get_user_token(response)
    another_profile = Profile(another_user)

    article = Article(another_profile)
    response = await create_article(service_client, article, another_user_token)
    assert response.status == HTTPStatus.OK

    commentList = CommentList(5, profile)
    for comment in commentList.comments:
        response = await add_comment(service_client, comment, article, user_token)
        assert response.status == HTTPStatus.OK

    await service_client.invalidate_caches()
    response = await get_comments(service_client, article, None)
    assert response.status == HTTPStatus.OK
    assert validate_comments(commentList, response)
