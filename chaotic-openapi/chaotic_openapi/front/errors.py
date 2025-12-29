from chaotic.front import parser

ERROR_MESSAGES = parser.ERROR_MESSAGES


def missing_field_msg(field: str) -> str:
    return ERROR_MESSAGES['missing'].format(field=field)


convert_error = parser.convert_error
