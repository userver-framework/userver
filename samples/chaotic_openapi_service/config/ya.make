UNION(generated-config)

SUBSCRIBER(g:taxi-common)

PEERDIR(
    taxi/uservices/userver/samples/chaotic_openapi_service/src
)

RUN_PROGRAM(
    taxi/uservices/userver/scripts/chaotic/merge_yaml_configs
        ${ARCADIA_BUILD_ROOT}/taxi/uservices/userver/samples/chaotic_openapi_service/src/config.chaotic.yaml
        ${ARCADIA_BUILD_ROOT}/taxi/uservices/userver/samples/chaotic_openapi_service/src/secure-gen/config.chaotic.yaml
        ${ARCADIA_ROOT}/taxi/uservices/userver/samples/chaotic_openapi_service/static_config.user.yaml
        -o ${BINDIR}/config.yaml
    IN_NOPARSE
        ${ARCADIA_BUILD_ROOT}/taxi/uservices/userver/samples/chaotic_openapi_service/src/config.chaotic.yaml
        ${ARCADIA_BUILD_ROOT}/taxi/uservices/userver/samples/chaotic_openapi_service/src/secure-gen/config.chaotic.yaml
        ${ARCADIA_ROOT}/taxi/uservices/userver/samples/chaotic_openapi_service/static_config.user.yaml
    OUT
        config.yaml
)

END()
