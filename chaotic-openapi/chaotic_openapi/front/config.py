import json
import mimetypes
from enum import Enum
from pathlib import Path
from typing import Optional, Union, Dict, List

from attr import define
from pydantic import BaseModel
from ruamel.yaml import YAML


class ClassOverride(BaseModel):
    """An override of a single generated class.

    See https://github.com/openapi-generators/openapi-python-client#class_overrides
    """

    class_name: Optional[str] = None
    module_name: Optional[str] = None


class MetaType(str, Enum):
    """The types of metadata supported for project generation."""

    NONE = "none"
    POETRY = "poetry"
    SETUP = "setup"
    PDM = "pdm"


class ConfigFile(BaseModel):
    """Contains any configurable values passed via a config file.

    See https://github.com/openapi-generators/openapi-python-client#configuration
    """

    class_overrides: Optional[Dict[str,  ClassOverride]] = None
    content_type_overrides: Optional[Dict[str,  str]] = None
    project_name_override: Optional[str] = None
    package_name_override: Optional[str] = None
    package_version_override: Optional[str] = None
    use_path_prefixes_for_title_model_names: bool = True
    post_hooks: Optional[List[str]] = None
    field_prefix: str = "field_"
    generate_all_tags: bool = False
    http_timeout: int = 5
    literal_enums: bool = False

    @staticmethod
    def load_from_path(path: Path) -> "ConfigFile":
        """Creates a Config from provided JSON or YAML file and sets a bunch of globals from it"""
        mime = mimetypes.guess_type(path.absolute().as_uri(), strict=True)[0]
        if mime == "application/json":
            config_data = json.loads(path.read_text())
        else:
            yaml = YAML(typ="safe")
            config_data = yaml.load(path)
        config = ConfigFile(**config_data)
        return config


@define
class Config:
    """Contains all the config values for the generator, from files, defaults, and CLI arguments."""

    meta_type: MetaType
    class_overrides: Dict[str,  ClassOverride]
    project_name_override: Optional[str]
    package_name_override: Optional[str]
    package_version_override: Optional[str]
    use_path_prefixes_for_title_model_names: bool
    post_hooks: List[str]
    field_prefix: str
    generate_all_tags: bool
    http_timeout: int
    literal_enums: bool
    document_source: Union[Path, str]
    file_encoding: str
    content_type_overrides: Dict[str,  str]
    overwrite: bool
    output_path: Optional[Path]

    @staticmethod
    def from_sources(
        config_file: ConfigFile,
        meta_type: MetaType,
        document_source: Union[Path, str],
        file_encoding: str,
        overwrite: bool,
        output_path: Optional[Path],
    ) -> "Config":
        if config_file.post_hooks is not None:
            post_hooks = config_file.post_hooks
        elif meta_type == MetaType.NONE:
            post_hooks = [
                "ruff check . --fix --extend-select=I",
                "ruff format .",
            ]
        else:
            post_hooks = [
                "ruff check --fix .",
                "ruff format .",
            ]

        config = Config(
            meta_type=meta_type,
            class_overrides=config_file.class_overrides or {},
            content_type_overrides=config_file.content_type_overrides or {},
            project_name_override=config_file.project_name_override,
            package_name_override=config_file.package_name_override,
            package_version_override=config_file.package_version_override,
            use_path_prefixes_for_title_model_names=config_file.use_path_prefixes_for_title_model_names,
            post_hooks=post_hooks,
            field_prefix=config_file.field_prefix,
            generate_all_tags=config_file.generate_all_tags,
            http_timeout=config_file.http_timeout,
            literal_enums=config_file.literal_enums,
            document_source=document_source,
            file_encoding=file_encoding,
            overwrite=overwrite,
            output_path=output_path,
        )
        return config
