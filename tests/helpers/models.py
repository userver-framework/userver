from pydantic import BaseModel, Field
from faker import Faker

fake = Faker()
fake.seed_instance(4321)


class User(BaseModel):
    username: str | None = Field(default_factory=fake.user_name)
    email: str | None = Field(default_factory=fake.email)
    password: str | None = Field(default_factory=fake.password)
    bio: str | None = Field(default_factory=fake.paragraph)
    image: str | None = Field(default_factory=fake.image_url)


class Profile(BaseModel):

    username: str | None
    bio: str | None
    image: str | None
    following: bool = False

    def __init__(self, user):
        username = user.username
        bio = user.bio
        image = user.image
