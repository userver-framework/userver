# pylint: disable=W0102(dangerous-default-value)
from typing import List
from typing import Optional

from pydantic import BaseModel
from pydantic import Field

from utils import fake
from utils import generate_title


class User(BaseModel):
    username: str | None = Field(default_factory=fake.user_name)
    email: str | None = Field(default_factory=fake.email)
    password: str | None = Field(default_factory=fake.password)
    bio: str | None = Field(default_factory=fake.paragraph)
    image: str | None = Field(default_factory=fake.image_url)


class Profile(BaseModel):

    username: Optional[str] = None
    bio: Optional[str] = None
    image: Optional[str] = None
    following: bool = False

    def __init__(self, user, *args, **kwargs):
        super().__init__(
            username=user.username,
            bio=user.bio,
            image=user.image,
            *args,
            **kwargs,
        )


class Article(BaseModel):

    slug: Optional[str] = None
    title: str = Field(default_factory=generate_title)
    description: str = Field(default_factory=fake.sentence)
    body: str = Field(default_factory=fake.paragraph)
    tagList: list = Field(default_factory=fake.words)
    favorited: bool = False
    favoritesCount: int = 0
    author: Optional[Profile] = None

    def __init__(self, *args, profile=None, **kwargs):
        super().__init__(*args, **kwargs)
        self.slug = self.title.lower().replace(' ', '-')
        self.author = profile

    def change_fields(self):
        new_article = Article()
        self.body = new_article.body
        self.description = new_article.description
        self.title = new_article.title
        self.slug = new_article.slug


class Comment(BaseModel):
    body: str = Field(default_factory=fake.sentence)
    author: Optional[Profile] = None

    def __init__(self, *args, profile=None, **kwargs):
        super().__init__(*args, **kwargs)
        self.author = profile


class CommentList(BaseModel):
    comments: List[Comment] = []

    def __init__(self, *args, n=1, profile=None, init_comments=[], **kwargs):
        super().__init__(*args, **kwargs)
        self.comments = init_comments + [
            Comment(profile) for i in range(n - len(init_comments))
        ]


class ArticleList(BaseModel):
    articles: List[Article] = []
    articlesCount: int = 0

    def __init__(self, *args, n=1, profile=None, init_articles=[], **kwargs):
        super().__init__(*args, **kwargs)
        self.articles = init_articles + [
            Article(profile) for i in range(n - len(init_articles))
        ]
