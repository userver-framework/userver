from typing import Any, Union, Dict
from openapi_python_client.parser.openapi import GeneratorData
from openapi_python_client.config import Config, ConfigFile, MetaType
from pathlib import Path

def parse(data_dict: Dict[str,  Any], config: Config):
    return GeneratorData.from_dict(data_dict, config=config)

def config():
    return Config.from_sources(ConfigFile(), MetaType.NONE, Path("opa.yaml"), 'utf-8', False, None)

from openapi_python_client import _get_document

conf = config()

# print(_get_document(source=conf.document_source, timeout=10))
print(parse(_get_document(source=conf.document_source, timeout=10), conf))
