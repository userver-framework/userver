from http import HTTPStatus

from endpoints import create_article
from endpoints import feed_articles
from endpoints import follow_user
from endpoints import register_user
from models import ArticleList
from models import Profile
from models import User
from utils import get_user_token
from validators import validate_articles


async def test_feed_articles_unauthorized(service_client):
    response = await feed_articles(service_client, None, 3, 0)
    assert response.status == HTTPStatus.UNAUTHORIZED


async def test_feed_articles(service_client):
    user = User(bio=None, image=None)
    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    user_token = get_user_token(response)

    another_user = User(bio=None, image=None)
    response = await register_user(service_client, another_user)
    assert response.status == HTTPStatus.OK

    another_user_token = get_user_token(response)

    user_profile = Profile(user)
    article_lst = ArticleList(5, user_profile)
    for article in article_lst.articles:
        response = await create_article(service_client, article, user_token)
        assert response.status == HTTPStatus.OK

    await service_client.invalidate_caches()
    response = await feed_articles(service_client, another_user_token, 2, 0)
    assert response.status == HTTPStatus.OK
    assert validate_articles(ArticleList(0), response)

    response = await follow_user(service_client, user, another_user_token)
    assert response.status == HTTPStatus.OK

    user_profile.following = True
    feed_articles_result = ArticleList(
        init_articles=article_lst.articles[:-3:-1],
    )
    await service_client.invalidate_caches()
    response = await feed_articles(service_client, another_user_token, 2, 0)
    assert response.status == HTTPStatus.OK
    assert validate_articles(feed_articles_result, response)
