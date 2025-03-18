PY3_PROGRAM()

PEERDIR(
    contrib/python/requests
    contrib/python/Jinja2
    taxi/github/code-linters/taxi_linters
)

PY_SRCS(
    __main__.py
)

RESOURCE_FILES(
    ya.make.jinja
)

END()
