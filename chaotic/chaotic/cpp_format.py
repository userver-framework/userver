import subprocess

SIMPLE_STYLE = '{BasedOnStyle: Google, ColumnLimit: 120}'


def format_pp(input_: str, *, binary: str) -> str:
    if not binary:
        return input_
    output = subprocess.check_output([binary, '--style', SIMPLE_STYLE], input=input_, encoding='utf-8')
    return output + '\n'
