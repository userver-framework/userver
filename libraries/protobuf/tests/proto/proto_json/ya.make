PROTO_LIBRARY()

SUBSCRIBER(g:taxi-common)

EXCLUDE_TAGS(GO_PROTO JAVA_PROTO PY_PROTO PY3_PROTO)
PROTO_NAMESPACE(taxi/uservices/userver/libraries/protobuf/tests/proto)

SRCS(
    messages.proto
)

END()
