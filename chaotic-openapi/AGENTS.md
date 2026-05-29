# chaotic-openapi — Agent Guide

## What this project is

OpenAPI 3.x / Swagger 2.0 → C++ HTTP client code generator for the userver framework,
plus a small C++ runtime library the generated code links against.

Main external dependencies:
- C++ code — `../chaotic`, `../core`, `../utils`
- cmake — `../cmake`, `../CMakeLists.txt`
- bash/python scripts, helpers — `../scripts`
- documentation in markdown — `../scripts/docs`


## References

Reference specification is located at github: https://github.com/OAI/OpenAPI-Specification/tree/main/versions


## Generation pipeline

```
OpenAPI/Swagger YAML
  → ref_resolver.sort_openapis()     topological sort of multi-file $refs
  → front/parser.Parser              pydantic objects → model.Service (language-neutral IR)
  → back/cpp/client/translator       model.Service → types.ClientSpec (C++-typed IR)
  → back/cpp/client/renderer         Jinja2 templates → 14 .hpp/.cpp files
```

`output.py` is **not** part of this pipeline. It is a separate Yandex build-system tool
that generates the `PEERDIR`/include lists for `ya.make` files. It is never called from
`main.py`.


# Tests

Tests are implemented at multiple levels:

- tests/front/ - python tests on OpenAPI/Swagger format parsing
- tests/back/ - python tests on IR -> C++-specific model translator
- integration_tests/ - full cycle tests (parsing -> translation -> rendering -> C++ compilation -> generated types tests)
- golden_tests/ - NOT tests, but a showcase of "what's possible with chaotic" / "how chaotic output looks like",
  containing examples of core types, cases and features


# Jinja style guide

See @AGENTS.jinja.md


# Python style guide

Style guide is based on PEP8.
Also:
- use `abc` module for abstract classes
- `from x import y` is limited to `y` modules (IMR241 check in flake8-import-restrictions)
