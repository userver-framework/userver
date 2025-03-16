from typing import Dict, List
from pydantic import BaseModel

class SecurityRequirement(BaseModel):
    __root__: Dict[str, List[str]]

    def __getitem__(self, key):
        return self.__root__[key]

    def __iter__(self):
        return iter(self.__root__)
