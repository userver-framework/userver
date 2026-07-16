"""Top-level configuration parsing for the changelog tool."""

import logging
import pathlib

import pydantic
import yaml

from changelog_tool.generate.config import GenerateConfig
from changelog_tool.llm.config import LLMConfig

logger = logging.getLogger(__name__)


class RenderConfig(pydantic.BaseModel):
    github_url: str
    github_token: str | None = None
    output_dir: pathlib.Path = pydantic.Field(default_factory=lambda: pathlib.Path('.changelog'))


class Config(pydantic.BaseModel):
    generate: GenerateConfig
    llm_config: LLMConfig = pydantic.Field(alias='llm-config')
    render: RenderConfig


def parse_config(config_path: pathlib.Path) -> Config:
    if not config_path.exists():
        raise FileNotFoundError(f'Configuration file not found: {config_path}')

    with open(config_path, encoding='utf-8') as f:
        yaml_data = yaml.safe_load(f)

    return Config.model_validate(yaml_data)
