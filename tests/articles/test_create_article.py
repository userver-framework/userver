import pytest
from http import HTTPStatus

from endpoints import register_user, get_profile, follow_user
from models import User, Profile
from validators import validate_profile
from utils import get_user_token
