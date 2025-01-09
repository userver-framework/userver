from typing import Optional

from pydantic import BaseModel, ConfigDict

from .external_documentation import ExternalDocumentation


class Tag(BaseModel):
    """
    Adds metadata to a single tag that is used by the [Operation Object](#operationObject).
    It is not mandatory to have a Tag Object per tag defined in the Operation Object instances.

    References:
        - https://swagger.io/docs/specification/paths-and-operations/
        - https://github.com/OAI/OpenAPI-Specification/blob/main/versions/3.0.3.md#tagObject
    """

    name: str
    description: Optional[str] = None
    externalDocs: Optional[ExternalDocumentation] = None
    model_config = ConfigDict(
        extra="allow", json_schema_extra={"examples": [{"name": "pet", "description": "Pets operations"}]}
    )
