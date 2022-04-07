#!/usr/bin/env python3

import argparse
import asyncio
import os
import pathlib
import re
import subprocess
from typing import Any
from typing import Dict
from typing import List

import aiohttp

WIKI_API_URL = 'https://wiki-api.yandex-team.ru/_api/frontend/'
WIKI_USERVER = 'wiki.yandex-team.ru/taxi/backend/userver'

DOT_FILES_PATH = 'dots'
WIKI_FILES_PATH = 'files'
DOXYGEN_CONF = 'doxygen.conf'
SCRIPTS_DOCS_PATH = os.path.join('scripts', 'docs')

IMAGE_EXTENSIONS = {'png', 'svg', 'jpg', 'jpeg', 'bmp', 'gif'}

DOT_REGEX = r'%%\(graphviz\)\n(.*?)\n%%'
INCLUDE_REGEX = r'{{[ ]*include[ ]+page=["\'](.*)["\'][ notitle notoc]*}}'
FILE_REFERENCE_REGEX = (
    r'(?:file:/taxi/backend|https?://jing.yandex-team.ru)/([^ \n})]*)'
)

# Reminder:
#     [\s\S] - matches any symbol, including newlines
#     \w == [a-zA-Z0-9_]
#     \S == [^ \t\n\r\f\v]
#     .  - does not match newlines
REGEXES = (
    #########################################################################
    # Doxygen specific ignores
    (r'%%\(wacko [ ]*wrapper=doxygen-ignore[ ]*\)[\s\S]*?%%', ''),
    #########################################################################
    # Highlighters converted to two horizontal separators
    (
        r'\n[ ]*%%\(wacko[^\)]*\)[ ]*\n([\s\S]*?)\n[ ]*%%',
        r'\n______ \n⚠️🐙❗ \1 \n______\n',  # Yep, emoji
    ),
    #########################################################################
    # Code block, single line
    (r'%%\((\w+)\)(.+?)%%', r'`\2`'),
    (r'%%(.+?)%%', r'`\1`'),
    #########################################################################
    # Code block, multi line
    (r'[ ]*\n[ ]*%%\((\w+)\)[ ]*\n([\s\S]*?)\n[ ]*%%', r'\n```\n\1\n\2\n```'),
    (r'[ ]*\n[ ]*%%[ ]*\n([\s\S]*?)\n[ ]*%%', r'\n```\nautodetect\n\1\n```'),
    #########################################################################
    # Numbered lists with parenthesis to lists with period: 42) => 42.
    (r'\n([ ]*\d+)\)', r'\n\1.'),
    #########################################################################
    # Headers with YaWiki anchors, dropping anchors: ==(link) X  =>  == X
    (r'(\n[=]+)\([^(][^)]*\)([^=\n]+)', r'\1\2'),
    #########################################################################
    # Headers
    (r'\n=======([^=\n]+)=======', r'\n\n###### \1'),
    (r'\n======([^=\n]+)======', r'\n\n##### \1'),
    (r'\n=====([^=\n]+)=====', r'\n\n#### \1'),
    (r'\n====([^=\n]+)====', r'\n\n### \1'),
    (r'\n===([^=\n]+)===', r'\n\n## \1'),
    (r'\n==([^=\n]+)==', r'\n\n# \1'),
    (r'\n=======', '\n\n###### '),
    (r'\n======', '\n\n##### '),
    (r'\n=====', '\n\n#### '),
    (r'\n====', '\n\n### '),
    (r'\n===', '\n\n## '),
    (r'\n==', '\n\n# '),
    #########################################################################
    # YaWiki links to links with absolute URL
    (r'\(\(/(\S*) (.+?)\)\)', r'((https://wiki.yandex-team.ru/\1 \2))'),
    #########################################################################
    # Anchors to current page, like ((#anchor_name Заголовок на странице))
    # Using JS to avoid URL change by Doxygen
    (
        r'\(\(#(\S*?) (.+?[\)]*)\)\)',
        '<a href="#" onclick=\'location.href="#\\1";return false;\'>\\2</a>',
    ),
    #########################################################################
    # Local links and dropping anchors :(
    (r'\(\(!/(\S*?)/?#\S+ (.+?[\)]*)\)\)', r'[\2](PY_HELPERS_FILENAME/\1.md)'),
    #########################################################################
    # Local links
    (r'\(\(!/(\S*?)/? (.+?[\)]*)\)\)', r'[\2](PY_HELPERS_FILENAME/\1.md)'),
    #########################################################################
    # Making links local and dropping anchors :(
    (
        r'\(\(https?://' + WIKI_USERVER + r'(\S*?)/?#\S+ (.+?[\)]*)\)\)',
        r'[\2](' + SCRIPTS_DOCS_PATH + r'/ru/userver\1.md)',
    ),
    #########################################################################
    # Making links local
    (
        r'\(\(https?://' + WIKI_USERVER + r'(\S*?)/? (.+?[\)]*)\)\)',
        r'[\2](' + SCRIPTS_DOCS_PATH + r'/ru/userver\1.md)',
    ),
    #########################################################################
    # Making external wiki-links non-local
    (r'\(\(/(\S+?)/ (.+?[\)]*)\)\)', r'[\2](https://wiki.yandex-team.ru/\1)'),
    #########################################################################
    # Links to Markdown format
    (r'\(\((\S+?) (.+?)\)\)', r'[\2](\1)'),
    #########################################################################
    # Italics
    (r' //(.+?)// ', r' _\1_ '),
    #########################################################################
    # Remove TOCs as we generate our own
    (r'{{[tT][oO][cC]}}', ''),
    (r'{{[ ]*tree.*?}}', ''),
    #########################################################################
    # We can not embed YaForms into Markdown
    (r'{{forms id=(\d+?)}}', r'https://forms.yandex.net/surveys/\1/'),
    #########################################################################
    # We can not make dropdowns in Doxygen
    (r'<{(.*)\n', r'**\1**\n'),
    (r'}>[ ]*\n', r'\n'),
    #########################################################################
    # Converting colored text into bold
    (r'!!\([^)]+\)([^!\n]*)!!', r'**⚠️🐙❗ \1**'),
    (r'!!([^!\n]*)!!', r'**⚠️🐙❗ \1**'),
    #########################################################################
    # Tables (see code in convert_to_markdown below for table tweaks)
    (r'\|\|(.*)\|\|\n', r'|\1|\n'),
    (r'\|#\n', ''),
    #########################################################################
)


async def request_yandex_service_data(
        token: str, url: str, *, binary: bool = False,
) -> Any:
    headers = {'Authorization': f'OAuth {token}'}

    async with aiohttp.ClientSession(
            headers=headers, requote_redirect_url=not binary,
    ) as session:
        async with session.get(url=url) as resp:
            if binary:
                return await resp.read()
            return await resp.json()


async def download_yawiki_page_data(
        token: str, page_url: str, add_header: bool = True,
) -> str:
    print(f'Downloading {page_url}')
    url = f'{WIKI_API_URL}taxi/backend/{page_url}/.raw?format=json'
    response_data = await request_yandex_service_data(token, url)

    data = response_data['data']
    body = data['body'].replace('\r', '')
    if 'title' in data and add_header:
        return '## ' + data['title'] + '\n' + body

    return body


async def download_yawiki_page_file(token: str, file_url: str) -> bytes:
    print(f'Downloading {file_url}')
    dirname = os.path.dirname(file_url)
    basename = os.path.basename(file_url)
    assert basename, f'URL "{file_url}" should end with file name'
    url = f'{WIKI_API_URL}taxi/backend/{dirname}/.files/{basename}'
    return await request_yandex_service_data(token, url, binary=True)


async def download_jing_file(token: str, file_url: str) -> bytes:
    print(f'Downloading {file_url}')
    return await request_yandex_service_data(
        token, f'https://jing.yandex-team.ru/{file_url}', binary=True,
    )


def unnest_pages(subpages: List[Dict[str, Any]]) -> List[str]:
    pages: List[str] = []
    for x in subpages:
        page = x['page']
        if page['type'] == 'P':
            pages.append(page['url'].replace('/taxi/backend/', ''))
        pages += unnest_pages(x['subpages'])
    return pages


async def download_page_tree(token: str) -> List[str]:
    url = f'{WIKI_API_URL}taxi/backend/userver/.tree?format=json&depth=100'
    response_data = await request_yandex_service_data(token, url)

    pages = unnest_pages(response_data['data']['subpages'])
    pages.append('userver')
    return pages


async def emplace_yawiki_files(data: str, token: str) -> str:
    if not os.path.exists(WIKI_FILES_PATH):
        os.makedirs(WIKI_FILES_PATH)

    for file_match in re.finditer(FILE_REFERENCE_REGEX, data):
        file_url = file_match.group(1)
        save_path = WIKI_FILES_PATH + '/' + file_url.replace('/', '_').lower()
        if not os.path.exists(save_path):
            if file_match.group(0).startswith('file:'):
                file_url = file_url.lower()
                response = await download_yawiki_page_file(token, file_url)
            else:
                response = await download_jing_file(token, file_url)

            with open(save_path, 'wb') as file:
                file.write(response)

        basename = os.path.basename(save_path)
        file_extension = basename.rsplit('.', 1)[-1].lower()
        if file_extension in IMAGE_EXTENSIONS:
            data = data.replace(file_match.group(0), f'![]({basename})')
        else:
            original_filename = os.path.basename(file_url)
            md_link = f'[{original_filename}]({basename})'
            data = data.replace(file_match.group(0), md_link)
    return data


async def emplace_yawiki_includes(data: str, token: str, page_url: str) -> str:
    includes_triggered = True
    while includes_triggered:
        includes_triggered = False
        for include in re.finditer(INCLUDE_REGEX, data):
            url = include.group(1)
            if url.startswith('!/'):
                url = url.replace('!', page_url, 1)
            elif url.startswith('/taxi/backend/'):
                url = url.replace('/taxi/backend/', '', 1)
            elif url.startswith('https://wiki.yandex-team.ru/taxi/backend/'):
                url = url.replace(
                    'https://wiki.yandex-team.ru/taxi/backend/', '', 1,
                )
            else:
                print(f'Changing inclusion of "{url}" to a link')
                data.replace(include.group(0), f'(({url} {url}))')
                continue

            include_data = await download_yawiki_page_data(
                token, url, add_header=False,
            )
            include_data.replace('\r', '')
            includes_triggered = True  # We do replacements recursively
            data = data.replace(include.group(0), include_data)

    return data


def graphviz_to_svg(data: str, filename: pathlib.Path) -> str:
    if not os.path.exists(DOT_FILES_PATH):
        os.makedirs(DOT_FILES_PATH)

    if not os.path.exists(WIKI_FILES_PATH):
        os.makedirs(WIKI_FILES_PATH)

    for i, dot in enumerate(re.finditer(DOT_REGEX, data, re.DOTALL)):
        new_filename = filename.with_name(filename.stem + '_' + str(i))
        new_filename = pathlib.Path(str(new_filename).replace('/', '_'))

        dot_filename = DOT_FILES_PATH / new_filename.with_suffix('.dot')
        svg_filename = WIKI_FILES_PATH / new_filename.with_suffix('.svg')

        with open(dot_filename, 'w', encoding='utf-8') as file:
            file.write(dot.group(1))

        subprocess.run(
            f'dot -Tsvg {dot_filename} -o {svg_filename}',
            capture_output=False,
            shell=True,
            encoding='utf-8',
        )

        data = data.replace(dot.group(0), f'![]({svg_filename.name})')
    return data


def convert_to_markdown(data: str, filename: pathlib.Path) -> str:
    # Print languages used in YaWiki
    # print(re.search(r'%%\(([a-zA-Z]+)\)\n', data)),
    data = graphviz_to_svg(data, filename)

    for pattern, replacement in REGEXES:
        data = re.sub(pattern, replacement, data)

    output = []
    table_started = False
    for line in data.split('\n'):
        if line == '#|':
            table_started = True
            continue

        output.append(line)

        if table_started:
            table_started = False
            delimiters_count = line.count('|') - 1
            output.append('|' + '-------------|' * delimiters_count)

    data = '\n'.join(output)

    local_file = str(filename.with_suffix('')).lower().replace('_', '')
    local_file = os.path.join(SCRIPTS_DOCS_PATH, local_file)
    data = data.replace('PY_HELPERS_FILENAME', local_file)
    for link in re.finditer(r']([^)\n]*)\.md\)', data):
        normalized_url = link.group(1).lower().replace('_', '')
        data = data.replace(link.group(1), normalized_url)
    return data


def convert_and_write_md(data: str, filename: pathlib.Path) -> None:
    with open(filename.with_suffix('.md'), 'w', encoding='utf-8') as file:
        file.write(convert_to_markdown(data, filename))


def get_fs_path_items(path: str) -> List[str]:
    if not os.path.exists(path):
        return []
    return [os.path.join(path, f) for f in os.listdir(path)]


def update_doxy_file(doxy_path: str) -> None:
    files = get_fs_path_items(WIKI_FILES_PATH)
    files = [''] + sorted(os.path.join(SCRIPTS_DOCS_PATH, x) for x in files)
    files_txt = '\nHTML_EXTRA_FILES =' + ' \\\n    '.join(files) + ' \\\n'

    with open(doxy_path, 'r', encoding='utf-8') as file:
        content = file.read()
    content = re.sub(r'\nHTML_EXTRA_FILES[ ]*=[ ]*\\\n', files_txt, content)
    with open(doxy_path, 'w', encoding='utf-8') as file:
        file.write(content)


async def generate_docs(
        token: str, ignores: List[str], additional_pages: List[str],
) -> None:
    all_pages = await download_page_tree(token)

    pages: List[str] = []
    pages.extend(additional_pages)
    for page in all_pages:
        if not any([page.startswith(ign) for ign in ignores]):
            pages.append(page)
    pages = sorted(set(pages))

    for page_url in pages:
        data = await download_yawiki_page_data(token, page_url)
        data = await emplace_yawiki_includes(data, token, page_url)
        data = await emplace_yawiki_files(data, token)

        filename = pathlib.Path(os.path.join('ru', page_url + '.yawiki'))
        if not os.path.exists(os.path.dirname(filename)):
            os.makedirs(os.path.dirname(filename))

        with open(filename, 'w', encoding='utf-8') as file:
            file.write(data)

        convert_and_write_md(data, filename)

    update_doxy_file(DOXYGEN_CONF)


def setup_workdir() -> None:
    abspath = os.path.abspath(__file__)
    dname = os.path.dirname(abspath)
    assert dname.endswith(
        SCRIPTS_DOCS_PATH,
    ), f'This script should be run from userver {SCRIPTS_DOCS_PATH}'

    os.chdir(dname)
    assert os.path.exists(
        DOXYGEN_CONF,
    ), f'doxygen.conf not found at {os.path.join(dname, DOXYGEN_CONF)}'


def main():
    parser = argparse.ArgumentParser(
        description='Download yawiki pages and convert them to Markdown',
    )
    parser.add_argument(
        '--token',
        required=True,
        help=(
            'OAuth token (you can get one from '
            'https://yql.yandex-team.ru/ -> gear -> "Give me my token!")'
        ),
    )
    parser.add_argument(
        '--convert-only',
        default=False,
        action='store_true',
        help='Do only the yaml2md conversion',
    )
    parser.add_argument(
        '--ignore-prefixes',
        nargs='+',
        default=['userver/libraries/'],
        help='Prefixes to ignore',
    )
    parser.add_argument(
        '--download-lists',
        nargs='+',
        default=['userver/libraries/yt', 'userver/libraries/yt-logger'],
        help='URLs to download, even if they hit the --ignore-prefixes',
    )
    args = parser.parse_args()

    setup_workdir()

    if args.convert_only:
        for filename in pathlib.Path('ru').rglob('*.yawiki'):
            with open(filename, 'r', encoding='utf-8') as source:
                convert_and_write_md(source.read(), filename)
        update_doxy_file(DOXYGEN_CONF)
        return

    loop = asyncio.get_event_loop()
    loop.run_until_complete(
        generate_docs(args.token, args.ignore_prefixes, args.download_lists),
    )


if __name__ == '__main__':
    main()
