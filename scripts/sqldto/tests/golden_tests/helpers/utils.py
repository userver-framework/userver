import contextlib
import dataclasses
import difflib
import pathlib
import sys


@contextlib.contextmanager
def mocked_argv(argv: list[str]):
    saved, sys.argv = sys.argv, argv
    try:
        yield
    finally:
        sys.argv = saved


@dataclasses.dataclass
class Case:
    input_dir: pathlib.Path
    output_dir: pathlib.Path
    migrations_dir_name: str = 'migrations'
    queries_dir_name: str = 'queries'

    @property
    def migrations_dir(self) -> pathlib.Path:
        return self.input_dir / self.migrations_dir_name

    @property
    def queries_dir(self) -> pathlib.Path:
        return self.input_dir / self.queries_dir_name

    @property
    def args(self) -> list[str]:
        assert self.input_dir.is_dir()
        assert self.output_dir.is_dir()

        args: list[str] = [
            '--dialect=postgresql',
            f'--namespace={self.input_dir.stem}',
            f'--output-dir={self.output_dir}',
            f'--dump-dir={self.input_dir}',
        ]

        if self.migrations_dir.exists():
            assert self.migrations_dir.is_dir()
            args.append(f'--migrations-dir={self.migrations_dir}')

        if self.queries_dir.exists():
            assert self.queries_dir.is_dir()
            args.append(f'--queries-dir={self.queries_dir}')

        return args

    def run(self) -> None:
        from sqldto.generator import main as generator_main

        with mocked_argv(['sqldto', *self.args]):
            assert generator_main.main() == 0, f'{self.input_dir.name}: generator failed'

    def dump(self) -> None:
        from sqldto.dumper import main as dumper_main

        with mocked_argv(['sqldto', *self.args]):
            assert dumper_main.main() == 0, f'{self.input_dir.name}: dumper failed'


def list_cases(input_dir: pathlib.Path) -> list[str]:
    return sorted(path.name for path in input_dir.iterdir() if path.is_dir())


def dirs_diff(expected_dir: pathlib.Path, actual_dir: pathlib.Path) -> str:
    def read_tree(root: pathlib.Path) -> dict[str, str]:
        return {
            str(path.relative_to(root)): path.read_text(encoding='utf-8')
            for path in sorted(root.rglob('*'))
            if path.is_file()
        }

    expected = read_tree(expected_dir)
    actual = read_tree(actual_dir)

    lines: list[str] = []
    for name in sorted(expected.keys() | actual.keys()):
        if expected.get(name) == actual.get(name):
            continue
        lines += difflib.unified_diff(
            expected.get(name, '').splitlines(),
            actual.get(name, '').splitlines(),
            fromfile=f'golden/{name}',
            tofile=f'generated/{name}',
            lineterm='',
        )
    return '\n'.join(lines)
