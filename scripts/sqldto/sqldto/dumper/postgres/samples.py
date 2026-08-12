import re

from sqldto.shared.types import models

_SAMPLES: list[tuple[re.Pattern, str]] = [
    # boolean
    (re.compile(r'^boolean$'), 'true'),
    # integers
    (re.compile(r'^smallint$'), '0'),
    (re.compile(r'^integer$'), '0'),
    (re.compile(r'^bigint$'), '0'),
    (re.compile(r'^smallserial$'), '0'),
    (re.compile(r'^serial$'), '0'),
    (re.compile(r'^bigserial$'), '0'),
    (re.compile(r'^oid$'), '0'),
    # floats
    (re.compile(r'^real$'), '0'),
    (re.compile(r'^double precision$'), '0'),
    # arbitrary-precision
    (re.compile(r'^numeric$'), '0'),
    (re.compile(r'^numeric\(\s*\d+\s*,\s*\d+\s*\)$'), '0'),
    (re.compile(r'^decimal$'), '0'),
    (re.compile(r'^decimal\(\s*\d+\s*,\s*\d+\s*\)$'), '0'),
    # text
    (re.compile(r'^char$'), "' '"),
    (re.compile(r'^character$'), "' '"),
    (re.compile(r'^character\(\d+\)$'), "' '"),
    (re.compile(r'^text$'), "''"),
    (re.compile(r'^character varying$'), "''"),
    (re.compile(r'^character varying\(\d+\)$'), "''"),
    # date/time
    (re.compile(r'^date$'), "'2000-01-01'"),
    (re.compile(r'^time without time zone$'), "'00:00:00'"),
    (re.compile(r'^time with time zone$'), "'00:00:00+00'"),
    (re.compile(r'^timestamp without time zone$'), "'2000-01-01 00:00:00'"),
    (re.compile(r'^timestamp with time zone$'), "'2000-01-01 00:00:00+00'"),
    (re.compile(r'^interval$'), "'0'"),
    # misc
    (re.compile(r'^uuid$'), "'00000000-0000-0000-0000-000000000000'"),
    (re.compile(r'^bytea$'), r"'\\x'"),
    (re.compile(r'^json$'), "'null'"),
    (re.compile(r'^jsonb$'), "'null'"),
    (re.compile(r'^inet$'), "'0.0.0.0'"),
    (re.compile(r'^cidr$'), "'0.0.0.0/32'"),
    (re.compile(r'^macaddr$'), "'00:00:00:00:00:00'"),
    (re.compile(r'^bit$'), "'0'"),
    (re.compile(r'^bit\(\d+\)$'), "'0'"),
    (re.compile(r'^bit varying$'), "'0'"),
    (re.compile(r'^bit varying\(\d+\)$'), "'0'"),
    (re.compile(r'^xml$'), "'<x/>'"),
]


def sample(param: models.QueryParam, catalog: models.Catalog) -> str:
    if not param.type:
        return 'NULL'

    if param.nullable:
        return 'NULL'

    if param.type.endswith('[]'):
        return "'{}'"

    if param.type in catalog.types:
        fields = catalog.types[param.type].fields
        inner = ', '.join(
            sample(
                models.QueryParam(
                    type=field.type,
                    nullable=False,
                ),
                catalog,
            )
            for field in fields
        )
        return f'ROW({inner})'

    if param.type in catalog.enums:
        return f"'{catalog.enums[param.type].values[0]}'"

    for pattern, value in _SAMPLES:
        if pattern.fullmatch(param.type):
            return f'{value}'

    return 'NULL'
