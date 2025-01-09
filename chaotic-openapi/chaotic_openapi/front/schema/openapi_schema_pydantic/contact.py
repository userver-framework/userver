from typing import Optional

from pydantic import BaseModel, ConfigDict


class Contact(BaseModel):
    """
    Contact information for the exposed API.

    See Also:
        - https://github.com/OAI/OpenAPI-Specification/blob/main/versions/3.0.3.md#contactObject
    """

    name: Optional[str] = None
    url: Optional[str] = None
    email: Optional[str] = None
    model_config = ConfigDict(
        extra="allow",
        json_schema_extra={
            "examples": [
                {"name": "API Support", "url": "http://www.example.com/support", "email": "support@example.com"}
            ]
        },
    )
