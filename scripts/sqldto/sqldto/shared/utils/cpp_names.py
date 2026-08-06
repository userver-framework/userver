from sqldto.shared.utils import cpp_keywords


def camel_case(string: str, no_lower_casing: bool = False) -> str:
    result = ''
    set_upper = True
    for char in string:
        if char in {'_', '-', '.'}:
            set_upper = True
        else:
            if set_upper:
                char = char.upper()
            elif not no_lower_casing:
                char = char.lower()
            result += char
            set_upper = False
    if not result and set_upper:
        result += '_'
    return result


def cpp_identifier(entity: str) -> str:
    entity = entity.rstrip('/')
    entity = ''.join(c if c.isalnum() else '_' for c in entity)

    if entity[:1].isnumeric():
        return 'x' + entity
    if not cpp_keywords.is_cpp_keyword(entity):
        return entity
    if entity == 'delete':
        return 'del'
    return entity + '_'


def cpp_identifier_lower(string: str) -> str:
    return cpp_identifier(string.lower())


def cpp_identifier_camel_case(
    string: str,
    no_lower_casing: bool = False,
) -> str:
    return cpp_identifier(camel_case(string, no_lower_casing=no_lower_casing))


def cpp_enum_entry(enum: str) -> str:
    return 'k' + camel_case(cpp_identifier(enum))
