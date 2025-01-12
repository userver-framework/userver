import sys
from typing import Union

import attr

from .properties import (
    ModelProperty,
    Property,
    Schemas,
    property_from_data,
)

from .. import schema as oai
from ..config import Config
from ..utils import get_content_type
from .errors import ErrorLevel, ParseError

if sys.version_info >= (3, 11):
    from enum import StrEnum

    class BodyType(StrEnum):
        JSON = "json"
        DATA = "data"
        FILES = "files"
        CONTENT = "content"
else:
    from enum import Enum

    class BodyType(str, Enum):
        JSON = "json"
        DATA = "data"
        FILES = "files"
        CONTENT = "content"


@attr.define
class Body:
    content_type: str
    prop: Property
    body_type: BodyType


def body_from_data(
    *,
    data: oai.Operation,
    schemas: Schemas,
    request_bodies: dict[str, Union[oai.RequestBody, oai.Reference]],
    config: Config,
    endpoint_name: str,
) -> tuple[list[Union[Body, ParseError]], Schemas]:
    """Adds form or JSON body to Endpoint if included in data"""
    body = _resolve_reference(data.request_body, request_bodies)
    if isinstance(body, ParseError):
        return [body], schemas
    if body is None:
        return [], schemas

    bodies: list[Union[Body, ParseError]] = []
    body_content = body.content
    prefix_type_names = len(body_content) > 1

    for content_type, media_type in body_content.items():
        simplified_content_type = get_content_type(content_type, config)
        if simplified_content_type is None:
            bodies.append(
                ParseError(
                    detail="Invalid content type",
                    data=body,
                    level=ErrorLevel.WARNING,
                )
            )
            continue
        media_type_schema = media_type.media_type_schema
        if media_type_schema is None:
            bodies.append(
                ParseError(
                    detail="Missing schema",
                    data=body,
                    level=ErrorLevel.WARNING,
                )
            )
            continue
        if simplified_content_type == "application/x-www-form-urlencoded":
            body_type = BodyType.DATA
        elif simplified_content_type == "multipart/form-data":
            body_type = BodyType.FILES
        elif simplified_content_type == "application/octet-stream":
            body_type = BodyType.CONTENT
        elif simplified_content_type == "application/json" or simplified_content_type.endswith("+json"):
            body_type = BodyType.JSON
        else:
            bodies.append(
                ParseError(
                    detail=f"Unsupported content type {simplified_content_type}",
                    data=body,
                    level=ErrorLevel.WARNING,
                )
            )
            continue
        prop, schemas = property_from_data(
            name="body",
            required=True,
            data=media_type_schema,
            schemas=schemas,
            parent_name=f"{endpoint_name}_{body_type}" if prefix_type_names else endpoint_name,
            config=config,
        )
        if isinstance(prop, ParseError):
            bodies.append(prop)
            continue
        if isinstance(prop, ModelProperty) and body_type == BodyType.FILES:
            # Regardless of if we just made this property or found it, it now needs the `to_multipart` method
            prop = attr.evolve(prop, is_multipart_body=True)
            schemas = attr.evolve(
                schemas,
                classes_by_name={
                    **schemas.classes_by_name,
                    prop.class_info.name: prop,
                },
                models_to_process=[*schemas.models_to_process, prop],
            )
        bodies.append(
            Body(
                content_type=content_type,
                prop=prop,
                body_type=body_type,
            )
        )

    return bodies, schemas


def _resolve_reference(
    body: Union[oai.RequestBody, oai.Reference, None], request_bodies: dict[str, Union[oai.RequestBody, oai.Reference]]
) -> Union[oai.RequestBody, ParseError, None]:
    if body is None:
        return None
    references_seen = []
    while isinstance(body, oai.Reference) and body.ref not in references_seen:
        references_seen.append(body.ref)
        body = request_bodies.get(body.ref.split("/")[-1])
    if isinstance(body, oai.Reference):
        return ParseError(detail="Circular $ref in request body", data=body)
    if body is None and references_seen:
        return ParseError(detail=f"Could not resolve $ref {references_seen[-1]} in request body")
    return body
