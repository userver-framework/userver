from pydantic import BaseModel
from typing import Optional

from contact import Contact
from license import License

class Info(BaseModel):
    title: str
    description: Optional[str] = None
    termsOfService: Optional[str] = None
    contact: Optional[Contact] = None
    license: Optional[License] = None
    version: str
