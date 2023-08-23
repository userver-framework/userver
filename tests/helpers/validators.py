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
    return validate_article_json(article, response.json()['article'])


def validate_article_json(article, response_json):
    response_json['tagList'] = set(
        response_json['tagList'])
    return response_json == {
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


def validate_articles(articleList, response):
    response_json = response.json()
    if int(response_json['articlesCount']) != len(articleList.articles):
        return False

    for article, json in zip(articleList.articles, response_json['articles']):
        res = validate_article_json(article, json)
        if not res:
            return False
    return True


def validate_comment_json(comment, json):
    return json == {
        'id': matching.positive_integer,
        'createdAt': matching.datetime_string,
        'updatedAt': matching.datetime_string,
        'body': comment.body,
        'author': comment.author.model_dump()
    }


def validate_comment(comment, response):
    return validate_comment_json(comment, response.json()['comment'])


def validate_comments(commentList, response):
    response_json = response.json()
    if len(response_json['comments']) != len(commentList.comments):
        return False
    for comment, json in zip(commentList.comments, response_json['comments']):
        res = validate_comment_json(comment, json)
        if not res:
            return False
    return True


def validate_tags(tagList, response):
    return set(response.json()["tags"]) == tagList
