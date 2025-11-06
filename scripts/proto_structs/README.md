# userver proto structs generator

The generator is currently highly experimental, cmake integration is non-existent.

To get proper syntax highlighting in an IDE, add `userver/scripts` directory to your Python Path. For example, in `.vscode/settings.json`:

```json
{
    "python.analysis.extraPaths": [
        "${workspaceFolder}/scripts"
    ],
    "python.languageServer": "Pylance"
}
```

# Implementation node: controlling whitespace in jinja

We want the generated files to look like a formatted file.
Applying clang-format is not an option, because it would take too long to build large projects.

First, read docs on [Whitespace Control](https://jinja.palletsprojects.com/en/stable/templates/#whitespace-control).
Summary, with our settings:

* Most whitespace is written as it appears in the template
   * This includes empty lines
   * This includes trailing newline after `}}` and `#}`

* A single trailing newline after `%}` is removed automatically

* `{%-`, `{{-`, `{#-` removes all whitespace before the block, starting from the previous text or statement

* `-%}`, `-}}`, `-#}` removes all whitespace after the block, ending at the next text or statement

* In particular, `{{-` and `-}}` are not equivalent to `trim`, they remove whitespace outside the expression

## Whitespace control policy

1. Do not indent anything manually. Use `{% filter indent(width=4, first=True) %}` instead.

2. Do not use `trim`. Instead of removing extra newlines, make it so that extra newlines are not generated.

3. A macro that generates a whole line or multiple lines should include the trailing newline character, 
   so that it produces a [POSIX line](https://stackoverflow.com/q/729692/5173839).

   * On the other hand, a macro that generates a part of a line should not include the trailing newline.
     Use `{%- endmacro %}` for that.

4. When calling macros that generate a whole line or multiple lines, be aware that `}}` produces an extra empty line
   (unless cut by the next block) use `-}}` or `{%-` in the next block to produce no extra empty lines.

5. For definitions that should be separated by a single empty line,
   put a single empty line at the beginning of the macro that generates it.

   * A block of such definitions looks like this:
     ```jinja
     {
     {% for child in children %}
     {{ render(child) -}}
     {% endfor %}

     }
     ```
     This expects that `render` macro includes a leading empty line.
   * This pattern composes nicely with complex nesting and elements of different kinds.
