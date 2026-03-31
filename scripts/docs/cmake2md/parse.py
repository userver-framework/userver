import dataclasses

from tree_sitter import Language
from tree_sitter import Node
from tree_sitter import Parser
from tree_sitter import Query
from tree_sitter import QueryCursor
from tree_sitter import Tree
import tree_sitter_cmake as tscmake

CMAKE_LANGUAGE = Language(tscmake.language())
PARSER = Parser(CMAKE_LANGUAGE)
FUNCTION_QUERY = Query(
    CMAKE_LANGUAGE,
    """
  (function_def
    (function_command
      (function)
      (argument_list
        (argument
          (unquoted_argument))) @arguments)
   ) @function_def
        """,
)
COMMAND_QUERY = Query(
    CMAKE_LANGUAGE,
    """
  (normal_command
    (identifier) @name
    (argument_list) @arguments) @command
        """,
)


@dataclasses.dataclass
class File:
    filepath: str
    content: bytes
    tree: Tree

    def get_text(self, node: Node) -> str:
        return self.content[node.start_byte : node.end_byte].decode('utf-8')


@dataclasses.dataclass
class Symbol:
    name: str
    type_: str
    groups: list[str]
    comments: list[str]


def get_comments(file: File, node: Node) -> list[str]:
    comments = []

    prev = node.prev_sibling
    while prev and prev.type == 'line_comment':
        comments.append(file.get_text(prev).removeprefix('#'))
        prev = prev.prev_sibling
    return list(reversed(comments))


def extract_functions(file: File) -> list[Symbol]:
    symbols = []

    query_cursor = QueryCursor(FUNCTION_QUERY)
    matches = query_cursor.matches(file.tree.root_node)
    for m in matches:
        argument_list = m[1]['arguments'][0]
        function = argument_list.children[0]
        text = file.get_text(function)

        function_def = m[1]['function_def'][0]
        comments = get_comments(file, function_def)

        symbols.append(
            Symbol(
                name=text,
                type_='function',
                groups=[],  # TODO
                comments=comments,
            )
        )
    return symbols


def extract_commands(file: File) -> list[Symbol]:
    symbols = []

    query_cursor = QueryCursor(COMMAND_QUERY)
    matches = query_cursor.matches(file.tree.root_node)
    for m in matches:
        command = m[1]['name'][0]
        command_text = file.get_text(command)

        arguments = m[1]['arguments'][0]
        args = [file.get_text(child) for child in arguments.children]
        comments = get_comments(file, m[1]['command'][0])

        symbols.append({
            'name': command_text,
            'args': args,
            'comments': comments,
        })
    return symbols


def parse_file(path: str) -> File:
    with open(path, encoding='utf-8') as ifile:
        content = bytes(ifile.read(), 'utf-8')
    tree = PARSER.parse(content)
    return File(filepath='', content=content, tree=tree)
