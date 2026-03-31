import json
from pathlib import Path
from urllib.error import HTTPError
from urllib.request import Request
from urllib.request import urlopen

url = 'https://api.github.com/repos/userver-framework/docs/contents/'
headers = {'Accept': 'application/vnd.github+json'}

try:
    request = Request(url, headers=headers)

    with urlopen(request) as response:
        data = json.loads(response.read().decode('utf-8'))

except HTTPError as e:
    print(f'HTTP Error {e.code}: {e.reason}')
    raise

versions = [item['name'] for item in data if item['type'] == 'dir' and item['name'].lower().startswith('v')]

versions.sort(key=lambda v: [int(x) for x in v.lstrip('vV').split('.')])

versions_js_path = Path('scripts/docs/scripts/versions.js')
versions_js_path.parent.mkdir(parents=True, exist_ok=True)

versions_js_content = f'export const versions = {json.dumps(versions)};\n'
versions_js_path.write_text(versions_js_content, encoding='utf-8')

versions_md_path = Path('scripts/docs/en/versions.md')
versions_md_path.parent.mkdir(parents=True, exist_ok=True)

versions_md_content_lines = (f'- [{ver}](docs/{ver}/index.html)' for ver in reversed(versions))
versions_md_content = (
    '\\page versions Versions\n\n- [trunk/develop](index.html)\n' + '\n'.join(versions_md_content_lines) + '\n'
)
versions_md_path.write_text(versions_md_content, encoding='utf-8')

print('✅ versions.js and versions.md generated')
