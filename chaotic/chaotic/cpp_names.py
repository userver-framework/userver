from . import cpp_keywords


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


def namespace(http_path: str) -> str:
    if http_path == '/':
        return 'root_'

    path = http_path.strip('/')
    for char in ('/', '.', '-', ':'):
        path = path.replace(char, '_')

    if path[0].isdigit():
        path = '_' + path

    if path[0] == '_':
        path = 'ns' + path

    path = path.lower()

    return cpp_identifier(path)
