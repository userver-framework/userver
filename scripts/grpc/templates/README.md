# gRPC code generation templates

The templates in this directory must render readable C++ with consistent
four-space block indentation, without a post-generation formatting step.
Running `clang-format` as part of code generation would add a noticeable cost
to every downstream build, while a format-only CI test would duplicate
formatter logic and require its own maintenance.

## Controlling whitespace

`generator.py` creates the Jinja environment with `trim_blocks=True` and
`lstrip_blocks=True`. With these settings, whitespace in template text is
emitted as written, while the newline immediately following a `{% ... %}` block
is removed. A minus sign in a Jinja delimiter (`{%-`, `-%}`, `{{-`, or `-}}`)
removes whitespace outside that block or expression.

When changing a template:

1. Keep rendered C++ indentation at four spaces.
2. Prefer reusable multiline macros and blocks with
   `{% filter indent(width=4, first=True) %}` instead of relying on indentation
   around the Jinja call site. Existing literal template output still has to
   carry the indentation expected in the generated C++.
3. Make a macro that renders complete lines include its trailing newline. A
   macro that renders only part of a line should not include one; use
   `{%- endmacro %}` in that case.
4. Do not use `trim`. Prevent unwanted whitespace at the producing block with
   the Jinja whitespace-control delimiters.
5. After changing whitespace, render representative unary, client-streaming,
   server-streaming, and bidirectional-streaming services, including the
   proto-struct variants, and inspect the affected generated blocks. A local
   comparison with the project `.clang-format` style can help find indentation
   drift, but differences in wrapping or single-line compaction are expected.

The proto-struct generator documents the same policy in more detail in
[its whitespace-control guide](../../proto_structs/README.md#implementation-node-controlling-whitespace-in-jinja).
