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


def validate_article(article, response):
    response_json = response.json()
    response_json['article']['tagList'] = set(
        response_json['article']['tagList'])
    return response_json == {
        'article': {
            'slug': article.slug,
            'title': article.title,
            'description': article.description,
            'body': article.body,
            'tagList': set(article.tagList),
            'createdAt': matching.datetime_string,
            'updatedAt': matching.datetime_string,
            'favorited': article.favorited,
            'favoritesCount': article.favoritesCount,
            'author': article.author.model_dump()
        }
    }


def validate_comment(comment, response):
    return response.json() == {
        'comment': {
            'id': matching.positive_integer,
            'createdAt': matching.datetime_string,
            'updatedAt': matching.datetime_string,
            'body': comment.body,
            'author': comment.author.model_dump()
        }
    }
