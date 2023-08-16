from testsuite.utils import matching
from models import *


def validate_user(user, response_json):
    return response_json == { 
        'user' : {
            'email' : user.email,
            'token' : matching.RegexString('^[\w-]+\.[\w-]+\.[\w-]+$'),
            'username': user.username,
            'bio' : user.bio,
            'image' : user.image,
        }
    }