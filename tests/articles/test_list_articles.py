import pytest
from http import HTTPStatus

from endpoints import register_user, create_article, follow_user, list_articles, favourite_article
from models import User, Profile, ArticleList
from validators import validate_articles
from utils import get_user_token


async def test_list_articles_unauthorized(service_client):
    response = await list_articles(service_client, None, 'tag', 'author', 'author', 2, 0)
    assert response.status == HTTPStatus.OK
    assert validate_articles(ArticleList(0), response)

    author = User(bio=None, image=None)
    response = await register_user(service_client, author)
    assert response.status == HTTPStatus.OK

    author_token = get_user_token(response)

    author_profile = Profile(author)

    tag = 'huba-buba'
    article_lst = ArticleList(5, author_profile)
    article_lst.articles[3].tagList.append(tag)
    article_lst.articles[4].tagList.append(tag)
    for article in article_lst.articles:
        response = await create_article(service_client, article, author_token)
        assert response.status == HTTPStatus.OK

    list_articles_result = ArticleList(
        init_articles=article_lst.articles[:-3:-1])

    another_user = User(bio=None, image=None)
    response = await register_user(service_client, another_user)
    assert response.status == HTTPStatus.OK

    another_user_token = get_user_token(response)

    for article in list_articles_result.articles:
        response = await favourite_article(service_client, article, another_user_token)
        article.favoritesCount = 1
        assert response.status == HTTPStatus.OK

    response = await list_articles(service_client, None, tag, author_profile.username, another_user.username, 2, 0)
    assert response.status == HTTPStatus.OK
    assert validate_articles(list_articles_result, response)


async def test_list_articles(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user_token = get_user_token(response)

    response = await list_articles(service_client, user_token, 'tag', 'author', 'author', 2, 0)
    assert response.status == HTTPStatus.OK
    assert validate_articles(ArticleList(0), response)

    author = User(bio=None, image=None)
    response = await register_user(service_client, author)
    assert response.status == HTTPStatus.OK

    author_token = get_user_token(response)

    author_profile = Profile(author)

    tag = 'huba-buba'
    article_lst = ArticleList(5, author_profile)
    article_lst.articles[3].tagList.append(tag)
    article_lst.articles[4].tagList.append(tag)
    for article in article_lst.articles:
        response = await create_article(service_client, article, author_token)
        assert response.status == HTTPStatus.OK

    list_articles_result = ArticleList(
        init_articles=article_lst.articles[:-3:-1])

    for article in list_articles_result.articles:
        response = await favourite_article(service_client, article, user_token)
        article.favoritesCount = 1
        article.favorited = True
        assert response.status == HTTPStatus.OK

    response = await follow_user(service_client, author, user_token)
    assert response.status == HTTPStatus.OK
    author_profile.following = True

    response = await list_articles(service_client, user_token, tag, author_profile.username, user.username, 2, 0)
    assert response.status == HTTPStatus.OK
    assert validate_articles(list_articles_result, response)
