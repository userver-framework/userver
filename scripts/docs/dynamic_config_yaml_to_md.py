#!/usr/bin/python

import argparse
import os
import pathlib

import yaml


def format_md(name, content, content_raw):
    description = content.get('description', '')
    example_text = content.get('schema', {}).get('example', '')
    if example_text:
        example = f'**Example:**\n```json\n{example_text}\n```'
    else:
        example = ''
    text = f"""@anchor {name}
## {name} Dynamic Config
{description}

**Schema:**
```
# yaml
{content_raw}
```

{example}
"""
    # TODO: used by component XX
    return text


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-o')
    parser.add_argument('yaml', nargs='+')
    return parser.parse_args()


def handle_file(ifname, ofname):
    with open(ifname) as ifile:
        content_raw = ifile.read()
        content = yaml.load(content_raw, yaml.Loader)

    md = format_md(pathlib.Path(ifname).stem, content, content_raw)

    with open(ofname, 'w') as ofile:
        ofile.write(md)


def main():
    args = parse_args()
    os.makedirs(args.o, exist_ok=True)
    all_configs = list()
    for fname in args.yaml:
        config_name = pathlib.Path(fname).stem
        handle_file(fname, pathlib.Path(args.o) / (config_name + '.md'))
        all_configs.append(config_name)

        with open(pathlib.Path(args.o) / 'dynamic_configs.md', 'w') as ofile:
            ofile.write(
                """## Dynamic config schemas list

Here you can find schemas of dynamic configs used by userver itself. For general
information on dynamic configs, see @ref scripts/docs/en/userver/dynamic_config.md

* @ref """
                + '\n* @ref '.join(sorted(all_configs))
                + """

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/dynamic_config.md | @ref scripts/docs/en/userver/log_level_running_service.md ⇨
@htmlonly </div> @endhtmlonly
"""
            )


if __name__ == '__main__':
    main()
