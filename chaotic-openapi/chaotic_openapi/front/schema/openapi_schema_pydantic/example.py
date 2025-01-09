from typing import Any, Optional

from pydantic import BaseModel, ConfigDict


class Example(BaseModel):
    """Examples added to parameters / components to help clarify usage.

    References:
        - https://swagger.io/docs/specification/adding-examples/
        - https://github.com/OAI/OpenAPI-Specification/blob/main/versions/3.0.3.md#exampleObject
    """

    summary: Optional[str] = None
    description: Optional[str] = None
    value: Optional[Any] = None
    externalValue: Optional[str] = None
    model_config = ConfigDict(
        extra="allow",
        json_schema_extra={
            "examples": [
                {"summary": "A foo example", "value": {"foo": "bar"}},
                {
                    "summary": "This is an example in XML",
                    "externalValue": "http://example.org/examples/address-example.xml",
                },
                {"summary": "This is a text example", "externalValue": "http://foo.bar/examples/address-example.txt"},
            ]
        },
    )
