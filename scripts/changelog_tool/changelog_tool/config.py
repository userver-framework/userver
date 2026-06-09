from changelog_tool.collect.config import CollectConfig

import pydantic
import yaml
import pathlib

class Config(pydantic.BaseModel):
    collect: CollectConfig

def parse_config(config_path: pathlib.Path) -> Config:
    with open(config_path, 'r') as f:
        yaml_data = yaml.safe_load(f)
        return Config.model_validate(yaml_data)