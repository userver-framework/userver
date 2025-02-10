from pydantic import BaseModel
from typing import Optional

class Info(BaseModel):
    title: str
    description: Optional[str] = None
    version: str
