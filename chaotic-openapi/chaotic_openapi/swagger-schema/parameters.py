from typing import Dict, List, Optional, Union
from pydantic import BaseModel, Field

class ParameterBase(BaseModel):
    name: str
    in_: str = Field(..., alias="in")
    description: Optional[str] = None
    required: Optional[bool] = None

class BodyParameter(ParameterBase):
    schema_: Dict = Field(..., alias="schema")

class NonBodyParameter(ParameterBase):
    type: str
    format: Optional[str] = None
    items: Optional[Dict] = None
    collectionFormat: Optional[str] = None
    default: Optional[Union[str, int, float, bool, List]] = None
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
    enum: Optional[List] = None
    multipleOf: Optional[float] = None

Parameter = Union[BodyParameter, NonBodyParameter]
Parameters = Dict[str, Parameter]
