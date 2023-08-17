from testsuite.utils import matching
from models import *


def validate_user(user, response):
    return response.json() == {
        'user': {
            'email': user.email,
            'token': matching.RegexString('^[\w-]+\.[\w-]+\.[\w-]+$'),
            'username': user.username,
            'bio': user.bio,
            'image': user.image,
        }
    }


def validate_profile(profile, response):
    return response.json() == {
        'profile': {
            'username': profile.username,
            'bio': profile.bio,
            'image': profile.image,
            'following': profile.following,
        }
    }
