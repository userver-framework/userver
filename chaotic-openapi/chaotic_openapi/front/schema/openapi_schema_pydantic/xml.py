from typing import Optional

from pydantic import BaseModel, ConfigDict


class XML(BaseModel):
    """
    A metadata object that allows for more fine-tuned XML model definitions.

    When using arrays, XML element names are *not* inferred (for singular/plural forms)
    and the `name` property SHOULD be used to add that information.
    See examples for expected behavior.

    References:
        - https://swagger.io/docs/specification/data-models/representing-xml/
        - https://github.com/OAI/OpenAPI-Specification/blob/main/versions/3.0.3.md#xmlObject
    """

    name: Optional[str] = None
    namespace: Optional[str] = None
    prefix: Optional[str] = None
    attribute: bool = False
    wrapped: bool = False
    model_config = ConfigDict(
        extra="allow",
        json_schema_extra={
            "examples": [
                {"namespace": "http://example.com/schema/sample", "prefix": "sample"},
                {"name": "aliens", "wrapped": True},
            ]
        },
    )
