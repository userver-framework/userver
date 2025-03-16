from typing import Dict, List, Optional
from pydantic import BaseModel, Field

class SecurityScheme(BaseModel):
    type: str
    description: Optional[str] = None
    name: Optional[str] = None
    in_: Optional[str] = Field(None, alias="in")
    flow: Optional[str] = None
    authorizationUrl: Optional[str] = None
    tokenUrl: Optional[str] = None
    scopes: Optional[Dict[str, str]] = None

class SecurityDefinitions(BaseModel):
    __root__: Dict[str, SecurityScheme]

    def __getitem__(self, key):
        return self.__root__[key]

    def __iter__(self):
        return iter(self.__root__)
