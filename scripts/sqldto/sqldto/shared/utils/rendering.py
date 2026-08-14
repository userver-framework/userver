from dataclasses import dataclass
from dataclasses import field
import functools
import pathlib

import jinja2

from sqldto.shared.loaders import fetcher
from sqldto.shared.types import models
from sqldto.shared.utils import cpp_format

ROOT_DIR = pathlib.Path(__file__).resolve().parent.parent.parent.parent


@dataclass
class ToGenerate:
    output_file: pathlib.Path
    loader: jinja2.BaseLoader | None = None
    template_name: str = ''
    context: dict = field(default_factory=dict)
    content: str | None = None
    clang_format: str | None = None
    includes: list[str] = field(default_factory=list)

    def render(self) -> str:
        if self.content is not None:
            content = self.content
        else:
            env = jinja2.Environment(
                loader=self.loader,
                trim_blocks=True,
                lstrip_blocks=True,
                keep_trailing_newline=True,
                autoescape=False,
            )
            render_context = {'includes': sorted(self.includes), **self.context}
            content = env.get_template(self.template_name).render(render_context)

        if self.clang_format:
            content = cpp_format.format_pp(
                content,
                binary=self.clang_format,
                output_file=self.output_file,
            )

        return content

    def write(self) -> None:
        self.output_file.parent.mkdir(parents=True, exist_ok=True)
        self.output_file.write_text(self.render(), encoding='utf-8')


@dataclass
class Context:
    namespace: str
    dialect: models.Dialect
    output_dir: pathlib.Path
    dump_dir: pathlib.Path
    migrations_dir: pathlib.Path
    migrations_output_dir: pathlib.Path | None
    queries_dir: pathlib.Path | None
    query_files: list[str]
    clang_format: str
    dump_command: str = ''
    schemas: models.Schema | None = field(default=None, init=False)

    @property
    def output_headers_dir(self) -> pathlib.Path:
        return self.output_dir / 'include' / self.namespace

    @property
    def output_sources_dir(self) -> pathlib.Path:
        return self.output_dir / 'src' / self.namespace

    def make_fetcher(self) -> fetcher.DumpLoader:
        query_files = self.query_files

        if self.queries_dir is not None and not query_files:
            query_files = [str(path) for path in self.queries_dir.rglob('*.sql')]

        loader = fetcher.DumpLoader(
            dump_dir=self.dump_dir,
            migrations_dir=self.migrations_dir,
            queries_dir=self.queries_dir,
            query_files=query_files,
            dump_command=self.dump_command,
        )
        loader.load()
        return loader

    @functools.cached_property
    def fetcher(self) -> fetcher.DumpLoader:
        return self.make_fetcher()

    @property
    def migrations(self) -> list[models.Migration]:
        return self.fetcher.migrations

    @property
    def queries(self) -> list[models.Query]:
        return self.fetcher.queries
