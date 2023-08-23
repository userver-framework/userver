import pytest
from http import HTTPStatus

from endpoints import register_user, create_article, add_comment, get_comments, delete_comment
from models import User, Profile, Article, Comment, CommentList
from validators import validate_comments
from utils import get_user_token


async def test_delete_self_comment(service_client):
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

    response = await delete_comment(service_client, 1, article, user_token)
    assert response.status == HTTPStatus.OK

    commentList = CommentList(0)
    await service_client.invalidate_caches()
    response = await get_comments(service_client, article, user_token)
    assert response.status == HTTPStatus.OK
    assert validate_comments(commentList, response)


async def test_delete_not_self_comment(service_client):
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

    comment = Comment(another_profile)
    response = await add_comment(service_client, comment, article, another_user_token)
    assert response.status == HTTPStatus.OK

    response = await delete_comment(service_client, 1, article, user_token)
    assert response.status == HTTPStatus.FORBIDDEN

    commentList = CommentList(init_comments=[comment])
    await service_client.invalidate_caches()
    response = await get_comments(service_client, article, user_token)
    assert response.status == HTTPStatus.OK
    assert validate_comments(commentList, response)


async def test_delete_comment_unknown_article(service_client):
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

    article.slug = 'invalid_slug'
    response = await delete_comment(service_client, 1, article, user_token)
    assert response.status == HTTPStatus.NOT_FOUND


async def test_delete_unknown_comment(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user_token = get_user_token(response)
    profile = Profile(user)
    article = Article(profile)
    response = await create_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK

    response = await delete_comment(service_client, 1, article, user_token)
    assert response.status == HTTPStatus.NOT_FOUND


async def test_delete_comment_unauthorized(service_client):
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

    response = await delete_comment(service_client, 1, article, None)
    assert response.status == HTTPStatus.UNAUTHORIZED
