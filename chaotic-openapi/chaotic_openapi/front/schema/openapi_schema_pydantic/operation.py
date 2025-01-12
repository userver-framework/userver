from typing import Optional, Union

from pydantic import BaseModel, ConfigDict, Field

from .callback import Callback
from .external_documentation import ExternalDocumentation
from .parameter import Parameter
from .reference import Reference
from .request_body import RequestBody
from .responses import Responses
from .security_requirement import SecurityRequirement
from .server import Server


class Operation(BaseModel):
    """Describes a single API operation on a path.

    References:
        - https://swagger.io/docs/specification/paths-and-operations/
        - https://github.com/OAI/OpenAPI-Specification/blob/main/versions/3.0.3.md#operationObject
    """

    tags: Optional[list[str]] = None
    summary: Optional[str] = None
    description: Optional[str] = None
    externalDocs: Optional[ExternalDocumentation] = None
    operationId: Optional[str] = None
    parameters: Optional[list[Union[Parameter, Reference]]] = None
    request_body: Optional[Union[RequestBody, Reference]] = Field(None, alias="requestBody")
    responses: Responses
    callbacks: Optional[dict[str, Callback]] = None

    deprecated: bool = False
    security: Optional[list[SecurityRequirement]] = None
    servers: Optional[list[Server]] = None
    model_config = ConfigDict(
        # `Callback` contains an unresolvable forward reference, will rebuild in `__init__.py`:
        defer_build=True,
        extra="allow",
        json_schema_extra={
            "examples": [
                {
                    "tags": ["pet"],
                    "summary": "Updates a pet in the store with form data",
                    "operationId": "updatePetWithForm",
                    "parameters": [
                        {
                            "name": "petId",
                            "in": "path",
                            "description": "ID of pet that needs to be updated",
                            "required": True,
                            "schema": {"type": "string"},
                        }
                    ],
                    "requestBody": {
                        "content": {
                            "application/x-www-form-urlencoded": {
                                "schema": {
                                    "type": "object",
                                    "properties": {
                                        "name": {
                                            "description": "Updated name of the pet",
                                            "type": "string",
                                        },
                                        "status": {
                                            "description": "Updated status of the pet",
                                            "type": "string",
                                        },
                                    },
                                    "required": ["status"],
                                }
                            }
                        }
                    },
                    "responses": {
                        "200": {
                            "description": "Pet updated.",
                            "content": {"application/json": {}, "application/xml": {}},
                        },
                        "405": {
                            "description": "Method Not Allowed",
                            "content": {"application/json": {}, "application/xml": {}},
                        },
                    },
                    "security": [{"petstore_auth": ["write:pets", "read:pets"]}],
                }
            ]
        },
    )
