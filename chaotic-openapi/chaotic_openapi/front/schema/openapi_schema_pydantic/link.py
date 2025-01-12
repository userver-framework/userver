from typing import Any, Optional

from pydantic import BaseModel, ConfigDict

from .server import Server


class Link(BaseModel):
    """
    The `Link object` represents a possible design-time link for a response.
    The presence of a link does not guarantee the caller's ability to successfully invoke it,
    rather it provides a known relationship and traversal mechanism between responses and other operations.

    Unlike _dynamic_ links (i.e. links provided **in** the response payload),
    the OAS linking mechanism does not require link information in the runtime response.

    For computing links, and providing instructions to execute them,
    a [runtime expression](#runtimeExpression) is used for accessing values in an operation
    and using them as parameters while invoking the linked operation.

    References:
        - https://swagger.io/docs/specification/links/
        - https://github.com/OAI/OpenAPI-Specification/blob/master/versions/3.0.3.md#linkObject
    """

    operationRef: Optional[str] = None
    operationId: Optional[str] = None
    parameters: Optional[dict[str, Any]] = None
    requestBody: Optional[Any] = None
    description: Optional[str] = None
    server: Optional[Server] = None
    model_config = ConfigDict(
        extra="allow",
        json_schema_extra={
            "examples": [
                {"operationId": "getUserAddressByUUID", "parameters": {"userUuid": "$response.body#/uuid"}},
                {
                    "operationRef": "#/paths/~12.0~1repositories~1{username}/get",
                    "parameters": {"username": "$response.body#/username"},
                },
            ]
        },
    )
