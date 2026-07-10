import pathlib
import subprocess

from sqldto.shared.utils import logging

DEFAULT_STYLE = '{BasedOnStyle: Google, ColumnLimit: 120}'
logger = logging.logger


def _clang_format_config(start: pathlib.Path) -> pathlib.Path | None:
    for parent in [start, *start.parents]:
        if (parent / '.clang-format').is_file():
            return parent / '.clang-format'
    return None


def format_pp(input: str, *, binary: str, output_file: pathlib.Path) -> str:
    if not binary:
        return input

    if _clang_format_config(output_file.parent):
        try:
            return (
                subprocess.check_output(
                    [
                        binary,
                        '--style=file',
                        f'--assume-filename={output_file}',
                    ],
                    input=input,
                    encoding='utf-8',
                )
                + '\n'
            )
        except subprocess.CalledProcessError:
            logger.debug('Failed to apply user .clang-format, using simple style')

    return (
        subprocess.check_output(
            [binary, f'--style={DEFAULT_STYLE}'],
            input=input,
            encoding='utf-8',
        )
        + '\n'
    )
