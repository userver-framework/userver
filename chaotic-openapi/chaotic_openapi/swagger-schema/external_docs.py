from typing import Optional
from pydantic import BaseModel

class ExternalDocs(BaseModel):
    url: str
    description: Optional[str] = None
