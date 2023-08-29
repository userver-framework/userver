from http import HTTPStatus

from endpoints import get_profile
from endpoints import register_user
from models import Profile
from models import User
from utils import get_user_token
from validators import validate_profile


async def test_get_profile_auth(service_client):
    user = User(bio=None, image=None)

    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    response = await get_profile(
        service_client, user, get_user_token(response),
    )
    assert response.status == HTTPStatus.OK

    profile = Profile(user)
    assert validate_profile(profile, response)


async def test_get_profile_unauthorized(service_client):
    user = User(bio=None, image=None)

    response = await register_user(service_client, user)
    assert response.status == HTTPStatus.OK

    response = await get_profile(service_client, user, None)
    assert response.status == HTTPStatus.OK

    profile = Profile(user)
    assert validate_profile(profile, response)
