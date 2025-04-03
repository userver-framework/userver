import pydantic

import chaotic.error

ERROR_MESSAGES = {
    'extra_forbidden': 'Extra fields are forbidden ({input})',
    'missing': 'Required field "{field}" is missing',
    'string_type': 'String type is expected, {input} is found',
    'bool_type': 'Boolean type is expected, {input} is found',
    'int_type': 'Integer type is expected, {input} is found',
}


def convert_error(full_filepath: str, schema_type: str, error: pydantic.ValidationError) -> chaotic.error.BaseError:
    assert len(error.errors()) >= 1

    # show only the first error
    error = error.errors()[0]

    # the last location is the missing field name
    field = error['loc'][-1]

    if error['type'] in ERROR_MESSAGES:
        msg = ERROR_MESSAGES[error['type']].format(**error, field=field)
    else:
        msg = error['msg']
    return chaotic.error.BaseError(
        full_filepath=full_filepath,
        infile_path='.'.join(error['loc']),
        schema_type=schema_type,
        msg=msg,
    )
