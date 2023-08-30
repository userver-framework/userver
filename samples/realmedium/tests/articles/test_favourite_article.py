from http import HTTPStatus

from endpoints import create_article
from endpoints import favourite_article
from endpoints import get_article
from endpoints import register_user
from models import Article
from models import Profile
from models import User
from utils import get_user_token
from validators import validate_article


async def test_favourite_article_unauthorized(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    article = Article(Profile(user))
    response = await create_article(
        service_client, article, get_user_token(response),
    )
    assert response.status == HTTPStatus.OK

    response = await favourite_article(service_client, article, None)
    assert response.status == HTTPStatus.UNAUTHORIZED


async def test_favourite_unknown_article(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    article = Article(Profile(user))

    response = await favourite_article(
        service_client, article, get_user_token(response),
    )
    assert response.status == HTTPStatus.NOT_FOUND


async def test_favourite_article(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user_token = get_user_token(response)

    article = Article(Profile(user))
    response = await create_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK

    another_user = User(bio=None, image=None)
    response = await register_user(service_client, another_user)
    assert response.status == HTTPStatus.OK

    another_user_token = get_user_token(response)

    response = await favourite_article(
        service_client, article, another_user_token,
    )
    assert response.status == HTTPStatus.OK
    article.favoritesCount = 1
    article.favorited = True
    assert validate_article(article, response)

    await service_client.invalidate_caches()
    response = await get_article(service_client, article, another_user_token)
    assert response.status == HTTPStatus.OK
    assert validate_article(article, response)

    article.favorited = False
    response = await get_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK
    assert validate_article(article, response)

    response = await get_article(service_client, article, None)
    assert response.status == HTTPStatus.OK
    assert validate_article(article, response)


async def test_self_favorite_article(service_client):

    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user_token = get_user_token(response)

    article = Article(Profile(user))
    response = await create_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK

    response = await favourite_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK
    article.favoritesCount = 1
    article.favorited = True
    assert validate_article(article, response)

    response = await favourite_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK
    assert validate_article(article, response)

    await service_client.invalidate_caches()
    response = await get_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK
    assert validate_article(article, response)


async def test_multiple_favorite_article(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user_token = get_user_token(response)

    article = Article(Profile(user))
    response = await create_article(service_client, article, user_token)
    assert response.status == HTTPStatus.OK
    article.favorited = True

    tokens = []
    for _ in range(3):
        user = User(bio=None, image=None)
        response = await register_user(service_client, user)
        assert response.status == HTTPStatus.OK
        tokens.append(get_user_token(response))

    for token in tokens:
        response = await favourite_article(service_client, article, token)
        assert response.status == HTTPStatus.OK
        article.favoritesCount += 1
        assert validate_article(article, response)

    await service_client.invalidate_caches()
    for token in tokens:
        response = await get_article(service_client, article, token)
        assert response.status == HTTPStatus.OK
        assert validate_article(article, response)
