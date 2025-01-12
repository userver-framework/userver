from typing import Any, Optional, Union

from pydantic import BaseModel, ConfigDict, Field

from .encoding import Encoding
from .example import Example
from .reference import Reference
from .schema import Schema


class MediaType(BaseModel):
    """Each Media Type Object provides schema and examples for the media type identified by its key.

    References:
        - https://swagger.io/docs/specification/media-types/
        - https://github.com/OAI/OpenAPI-Specification/blob/main/versions/3.0.3.md#mediaTypeObject
    """

    media_type_schema: Optional[Union[Reference, Schema]] = Field(default=None, alias="schema")
    example: Optional[Any] = None
    examples: Optional[dict[str, Union[Example, Reference]]] = None
    encoding: Optional[dict[str, Encoding]] = None
    model_config = ConfigDict(
        # `Encoding` is not build yet, will rebuild in `__init__.py`:
        defer_build=True,
        extra="allow",
        populate_by_name=True,
        json_schema_extra={
            "examples": [
                {
                    "schema": {"$ref": "#/components/schemas/Pet"},
                    "examples": {
                        "cat": {
                            "summary": "An example of a cat",
                            "value": {
                                "name": "Fluffy",
                                "petType": "Cat",
                                "color": "White",
                                "gender": "male",
                                "breed": "Persian",
                            },
                        },
                        "dog": {
                            "summary": "An example of a dog with a cat's name",
                            "value": {
                                "name": "Puma",
                                "petType": "Dog",
                                "color": "Black",
                                "gender": "Female",
                                "breed": "Mixed",
                            },
                        },
                    },
                }
            ]
        },
    )
