from typing import Optional

from pydantic import BaseModel, ConfigDict

from .contact import Contact
from .license import License


class Info(BaseModel):
    """
    The object provides metadata about the API.
    The metadata MAY be used by the clients if needed,
    and MAY be presented in editing or documentation generation tools for convenience.

    References:
        - https://swagger.io/docs/specification/api-general-info/
        -https://github.com/OAI/OpenAPI-Specification/blob/main/versions/3.0.3.md#infoObject
    """

    title: str
    description: Optional[str] = None
    termsOfService: Optional[str] = None
    contact: Optional[Contact] = None
    license: Optional[License] = None
    version: str
    model_config = ConfigDict(
        extra="allow",
        json_schema_extra={
            "examples": [
                {
                    "title": "Sample Pet Store App",
                    "description": "This is a sample server for a pet store.",
                    "termsOfService": "http://example.com/terms/",
                    "contact": {
                        "name": "API Support",
                        "url": "http://www.example.com/support",
                        "email": "support@example.com",
                    },
                    "license": {"name": "Apache 2.0", "url": "https://www.apache.org/licenses/LICENSE-2.0.html"},
                    "version": "1.0.1",
                }
            ]
        },
    )
