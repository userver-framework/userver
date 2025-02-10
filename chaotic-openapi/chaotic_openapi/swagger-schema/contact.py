from pydantic import BaseModel
from typing import Optional

class Contact(BaseModel):
    name: Optional[str] = None
    url: Optional[str] = None
    email: Optional[str] = None
