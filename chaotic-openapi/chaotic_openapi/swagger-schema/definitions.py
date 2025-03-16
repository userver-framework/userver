from typing import Dict, List, Optional, Union, Any
from pydantic import BaseModel, Field

class Schema(BaseModel):
    type: Optional[str] = None
    format: Optional[str] = None
    items: Optional[Union[Dict, 'Schema']] = None
    properties: Optional[Dict[str, Union[Dict, 'Schema']]] = None
    required: Optional[List[str]] = None
    description: Optional[str] = None
    default: Optional[Any] = None
    allOf: Optional[List[Union[Dict, 'Schema']]] = None
    oneOf: Optional[List[Union[Dict, 'Schema']]] = None
    anyOf: Optional[List[Union[Dict, 'Schema']]] = None
    not_: Optional[Union[Dict, 'Schema']] = Field(None, alias="not")
    additionalProperties: Optional[Union[bool, Dict, 'Schema']] = None
    enum: Optional[List[Any]] = None
    multipleOf: Optional[float] = None
    maximum: Optional[float] = None
    exclusiveMaximum: Optional[bool] = None
    minimum: Optional[float] = None
    exclusiveMinimum: Optional[bool] = None
    maxLength: Optional[int] = None
    minLength: Optional[int] = None
    pattern: Optional[str] = None
    maxItems: Optional[int] = None
    minItems: Optional[int] = None
    uniqueItems: Optional[bool] = None
    maxProperties: Optional[int] = None
    minProperties: Optional[int] = None
    ref: Optional[str] = Field(None, alias="$ref")

Schema.model_rebuild()

class Definitions(BaseModel):
    __root__: Dict[str, Schema]

    def __getitem__(self, key):
        return self.__root__[key]

    def __iter__(self):
        return iter(self.__root__)
