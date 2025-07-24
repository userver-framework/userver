PY3_LIBRARY()

SUBSCRIBER(
    g:taxi-common
)

PEERDIR(
    contrib/python/Jinja2
    contrib/python/protobuf
)

RESOURCE_FILES(
    PREFIX taxi/uservices/userver/scripts/proto_structs/
    templates/structs.usrv.cpp.jinja
    templates/structs.usrv.hpp.jinja
    templates/utils.inc.jinja
)

PY_SRCS(
    __init__.py
    generator.py
)

END()
