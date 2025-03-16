from typing import Any, Dict, List, Optional, Union
from pydantic import BaseModel

class Response(BaseModel):
    description: str
    schema: Optional[Dict] = None
    headers: Optional[Dict[str, Dict]] = None
    examples: Optional[Dict[str, Any]] = None

class Responses(BaseModel):
    __root__: Dict[str, Response]

    def __getitem__(self, key):
        return self.__root__[key]

    def __iter__(self):
        return iter(self.__root__)
