from pydantic import BaseModel
from typing import Optional

class License(BaseModel):
    name: str
    url: Optional[str] = None
