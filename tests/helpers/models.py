from typing import Optional
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

    username: Optional[str] = None
    bio: Optional[str] = None
    image: Optional[str] = None
    following: bool = False

    def __init__(self, user, *args, **kwargs):
        super().__init__(
            username = user.username,
            bio = user.bio,
            image = user.image,
            *args, **kwargs
        )
