from typing import Any, Union, Dict
from front.parser import GeneratorData
from front.config import Config, ConfigFile, MetaType
from pathlib import Path

def parse(data_dict: Dict[str,  Any], config: Config):
    return GeneratorData.from_dict(data_dict, config=config)

def config():
    return Config.from_sources(ConfigFile(), MetaType.NONE, Path("opa.yaml"), 'utf-8', False, None)

from front import _get_document

conf = config()

print(parse(_get_document(conf.document_source), conf))
