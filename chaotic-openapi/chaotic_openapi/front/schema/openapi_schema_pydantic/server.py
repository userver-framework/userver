from typing import Optional

from pydantic import BaseModel, ConfigDict

from .server_variable import ServerVariable


class Server(BaseModel):
    """An object representing a Server.

    References:
        - https://swagger.io/docs/specification/api-host-and-base-path/
        - https://github.com/OAI/OpenAPI-Specification/blob/main/versions/3.0.3.md#serverObject
    """

    url: str
    description: Optional[str] = None
    variables: Optional[dict[str, ServerVariable]] = None
    model_config = ConfigDict(
        extra="allow",
        json_schema_extra={
            "examples": [
                {"url": "https://development.gigantic-server.com/v1", "description": "Development server"},
                {
                    "url": "https://{username}.gigantic-server.com:{port}/{basePath}",
                    "description": "The production API server",
                    "variables": {
                        "username": {
                            "default": "demo",
                            "description": "this value is assigned by the service provider, "
                            "in this example `gigantic-server.com`",
                        },
                        "port": {"enum": ["8443", "443"], "default": "8443"},
                        "basePath": {"default": "v2"},
                    },
                },
            ]
        },
    )
