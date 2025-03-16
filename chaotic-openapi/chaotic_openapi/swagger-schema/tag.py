from typing import Optional
from pydantic import BaseModel
from external_docs import ExternalDocs

class Tag(BaseModel):
    name: str
    description: Optional[str] = None
    externalDocs: Optional[ExternalDocs] = None
