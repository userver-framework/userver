from changelog_tool.collect.config import CollectConfig
from changelog_tool.llm.config import LLMConfig

import pydantic
import yaml
import pathlib

class Config(pydantic.BaseModel):
    collect: CollectConfig
    llm_config: LLMConfig = pydantic.Field(alias="llm-config")

def parse_config(config_path: pathlib.Path) -> Config:
    with open(config_path, 'r') as f:
        yaml_data = yaml.safe_load(f)
        return Config.model_validate(yaml_data)