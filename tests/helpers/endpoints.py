from utils import *


async def register_user(service_client, user):
    return await service_client.post(
        Routes.REGISTRATION,
        json=model_dump(user, include=RequiredFields.REGISTRATION.value)
    )


async def login_user(service_client, user):
    return await service_client.post(
        Routes.LOGIN,
        json=model_dump(user, include=RequiredFields.LOGIN.value)
    )


async def get_user(service_client, token):
    return await service_client.get(
        Routes.GET_USER,
        headers={'Authorization': token},
    )


async def update_user(service_client, user, token):
    return await service_client.put(
        Routes.UPDATE_USER,
        json=model_dump(user, exclude_none=True),
        headers={'Authorization': token},
    )


async def get_profile(service_client, user, token):
    return await service_client.get(
        Routes.GET_PROFILE.format(username=user.username),
        headers={'Authorization': token}
    )


async def follow_user(service_client, user, token):
    return await service_client.post(
        Routes.FOLLOW_PROFILE.format(username=user.username),
        headers={'Authorization': token},
    )


async def unfollow_user(service_client, user, token):
    return await service_client.delete(
        Routes.UNFOLLOW_PROFILE.format(username=user.username),
        headers={'Authorization': token},
    )


async def create_article(service_client, article, token):
    return await service_client.post(
        Routes.CREATE_ARTICLE,
        json=model_dump(
            article, include=RequiredFields.CREATE_ARTICLE.value, exclude_none=True),
        headers={'Authorization': token},
    )


async def get_article(service_client, article, token):
    return await service_client.get(
        Routes.GET_ARTICLE.format(slug=article.slug),
        headers={'Authorization': token},
    )


async def add_comment(service_client, comment, article, token):
    return await service_client.post(
        Routes.ADD_COMMENT.format(slug=article.slug),
        json=model_dump(comment, include=RequiredFields.ADD_COMMENT.value),
        headers={'Authorization': token},
    )