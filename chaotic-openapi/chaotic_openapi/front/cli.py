import codecs
from collections.abc import Sequence
from pathlib import Path
from pprint import pformat
from typing import Optional, Union

import typer

from openapi_python_client import MetaType
from openapi_python_client.config import Config, ConfigFile
from openapi_python_client.parser.errors import ErrorLevel, GeneratorError, ParseError

def _process_config(
    *,
    url: Optional[str],
    path: Optional[Path],
    config_path: Optional[Path],
    meta_type: MetaType,
    file_encoding: str,
    overwrite: bool,
    output_path: Optional[Path],
) -> Config:
    source: Union[Path, str]
    if url and not path:
        source = url
    elif path and not url:
        source = path
    elif url and path:
        typer.secho("Provide either --url or --path, not both", fg=typer.colors.RED)
        raise typer.Exit(code=1)
    else:
        typer.secho("You must either provide --url or --path", fg=typer.colors.RED)
        raise typer.Exit(code=1)

    try:
        codecs.getencoder(file_encoding)
    except LookupError as err:
        typer.secho(f"Unknown encoding : {file_encoding}", fg=typer.colors.RED)
        raise typer.Exit(code=1) from err

    if not config_path:
        config_file = ConfigFile()
    else:
        try:
            config_file = ConfigFile.load_from_path(path=config_path)
        except Exception as err:
            raise typer.BadParameter("Unable to parse config") from err

    return Config.from_sources(config_file, meta_type, source, file_encoding, overwrite, output_path=output_path)
