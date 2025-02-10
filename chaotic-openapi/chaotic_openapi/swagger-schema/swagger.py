from pydantic import BaseModel, Field
from typing import Any, Dict, List, Optional
import json, yaml

from info import Info
from paths import Paths

class SwaggerSchema(BaseModel):
    swagger: str = Field("2.0", literal=True)
    info: Info
    host: Optional[str] = None
    basePath: Optional[str] = None
    schemes: Optional[List[str]] = None
    consumes: Optional[List[str]] = None
    produces: Optional[List[str]] = None
    paths: Paths

def parse_swagger_file(file_path: str) -> SwaggerSchema:
    with open(file_path, "r") as file:
        data = yaml.load(file)
    return SwaggerSchema(**data)

parsed_schema = parse_swagger_file("swagger_example.yaml")
print(parsed_schema)
