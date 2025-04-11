import subprocess


def format_pp(input_: str, *, binary: str) -> str:
    if not binary:
        return input_
    output = subprocess.check_output([binary], input=input_, encoding='utf-8')
    return output + '\n'
