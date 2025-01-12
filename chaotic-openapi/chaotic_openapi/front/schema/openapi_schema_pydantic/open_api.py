from typing import Optional

from pydantic import BaseModel, ConfigDict, field_validator

from .components import Components
from .external_documentation import ExternalDocumentation
from .info import Info
from .paths import Paths
from .security_requirement import SecurityRequirement
from .server import Server
from .tag import Tag

NUM_SEMVER_PARTS = 3


class OpenAPI(BaseModel):
    """This is the root document object of the OpenAPI document.

    References:
        - https://github.com/OAI/OpenAPI-Specification/blob/main/versions/3.0.3.md#oasObject
        - https://swagger.io/docs/specification/basic-structure/
    """

    info: Info
    servers: list[Server] = [Server(url="/")]
    paths: Paths
    components: Optional[Components] = None
    security: Optional[list[SecurityRequirement]] = None
    tags: Optional[list[Tag]] = None
    externalDocs: Optional[ExternalDocumentation] = None
    openapi: str
    model_config = ConfigDict(
        # `Components` is not build yet, will rebuild in `__init__.py`:
        defer_build=True,
        extra="allow",
    )

    @field_validator("openapi")
    @classmethod
    def check_openapi_version(cls, value: str) -> str:
        """Validates that the declared OpenAPI version is a supported one"""
        parts = value.split(".")
        if len(parts) != NUM_SEMVER_PARTS:
            raise ValueError(f"Invalid OpenAPI version {value}")
        if parts[0] != "3":
            raise ValueError(f"Only OpenAPI versions 3.* are supported, got {value}")
        if int(parts[1]) > 1:
            raise ValueError(f"Only OpenAPI versions 3.1.* are supported, got {value}")
        return value
