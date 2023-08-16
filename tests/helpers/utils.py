from enum import Enum


class Routes(str, Enum):
    LOGIN = '/api/users/login'
    REGISTRATION = '/api/users'
    GET_USER = '/api/user'
    UPDATE_USER = '/api/user'
    GET_PROFILE = '/api/profiles/{username}'
    FOLLOW_PROFILE = '/api/profiles/:username/follow'
    UNFOLLOW_PROFILE = '/api/profiles/:username/follow'
    LIST_ARTICLES = '/api/articles'
    FEED_ARTICLES = '/api/articles/feed'
    GET_ARTICLE = '/api/articles/:slug'
    CREATE_ARTICLE = '/api/articles'
    UPDATE_ARTICLE = '/api/articles/:slug'
    DELETE_ARTICLE = '/api/articles/:slug'
    ADD_COMMENT = '/api/articles/:slug/comments'
    GET_COMMENTS = '/api/articles/:slug/comments'
    DELETE_COMMENT = '/api/articles/:slug/comments/:id'
    FAVOURITE_ARTICLE = '/api/articles/:slug/favorite'
    UNFAVOURITE_ARTICLE = '/api/articles/:slug/favorite'
    GET_TAGS = '/api/tags'

    def __str__(self) -> str:
        return self.value


class RequiredFields(tuple, Enum):
    LOGIN = 'email', 'password'
    REGISTRATION = 'username', 'email', 'password'


def model_dump(model, **kwargs):
    return {model.__class__.__name__.lower(): model.model_dump(**kwargs)}


def get_user_token(response):
    return 'Token {token}'.format(token=response.json()['user']['token'])
