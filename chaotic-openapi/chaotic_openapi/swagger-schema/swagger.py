from pydantic import BaseModel, Field
from typing import Any, Dict, List, Optional
import json, yaml

from info import Info
from paths import Paths
from definitions import Definitions
from parameters import Parameters
from responses import Responses
from security_definitions import SecurityDefinitions
from security_requirement import SecurityRequirement
from tag import Tag
from external_docs import ExternalDocs

class SwaggerSchema(BaseModel):
    swagger: str = Field("2.0", literal=True)
    info: Info
    host: Optional[str] = None
    basePath: Optional[str] = None
    schemes: Optional[List[str]] = None
    consumes: Optional[List[str]] = None
    produces: Optional[List[str]] = None
    paths: Paths
    definitions : Optional[Definitions] = None
    parameters : Optional[Parameters] = None
    responses : Optional[Responses] = None
    securityDefinitions : Optional[SecurityDefinitions] = None
    security : Optional[List[SecurityRequirement]] = None
    tags : Optional[List[Tag]] = None
    externalDocs : Optional[ExternalDocs] = None

def parse_swagger_file(file_path: str) -> SwaggerSchema:
    with open(file_path, "r") as file:
        data = yaml.safe_load(file)
    return SwaggerSchema(**data)

parsed_schema = parse_swagger_file("swagger_example.yaml")
print(parsed_schema)
