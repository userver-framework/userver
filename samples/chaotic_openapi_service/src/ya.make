LIBRARY(userver-functional-test-service-lib)

SUBSCRIBER(g:taxi-common)

PEERDIR(
    taxi/uservices/userver/core
    taxi/uservices/userver/chaotic-openapi
)

INCLUDE(${ARCADIA_ROOT}/taxi/uservices/userver/chaotic/ya.make.deps)
INCLUDE(${ARCADIA_ROOT}/taxi/uservices/userver/chaotic-openapi/ya.make.deps)

ADDINCL(
    GLOBAL taxi/uservices/userver/samples/chaotic_openapi_service/src
)

SRCS(
    hello_handler.cpp
    say_hello.cpp
    handlers/insecure/insecuresecretpost/view.cpp
)

ADDINCL(
    GLOBAL ${ARCADIA_BUILD_ROOT}/${MODDIR}/include
)
RUN_PROGRAM(
    taxi/uservices/userver/chaotic-openapi/bin
        --name test
        --gen client
        --dynamic-config TEST_CLIENT_QOS
        -o ${BINDIR}/
        --clang-format ''
        ../clients/test.yaml
    OUTPUT_INCLUDES
        ${CHAOTIC_INCLUDES}
        ${CHAOTIC_OPENAPI_INCLUDES}
    IN_NOPARSE
        ../clients/test.yaml
    OUT
        src/clients/test/client_impl.cpp
        src/clients/test/client.cpp
        src/clients/test/requests.cpp
        src/clients/test/responses.cpp
        src/clients/test/component.cpp
        src/clients/test/exceptions.cpp

        include/clients/test/client_impl.hpp
        include/clients/test/client.hpp
        include/clients/test/requests.hpp
        include/clients/test/responses.hpp
        include/clients/test/exceptions.hpp
        include/clients/test/component.hpp
        include/clients/test/qos.hpp

        src/clients/test/test.cpp
        include/clients/test/test.hpp
        include/clients/test/test_fwd.hpp
        include/clients/test/test_parsers.ipp
        include/clients/test/test_sax_parsers.hpp
)

RUN_PROGRAM(
    taxi/uservices/userver/chaotic-openapi/bin
        --name insecure
        --gen handlers
        -o ${BINDIR}/
        --clang-format ''
        ../handlers/insecure/openapi.yaml
    OUTPUT_INCLUDES
        ${CHAOTIC_INCLUDES}
        ${CHAOTIC_OPENAPI_INCLUDES}
        handlers/insecure/insecuresecretpost/view.hpp
    IN_NOPARSE
        ../handlers/insecure/openapi.yaml
    OUT
        include/handlers/insecure/openapi.hpp
        include/handlers/insecure/openapi_fwd.hpp
        include/handlers/insecure/openapi_parsers.ipp
        include/handlers/insecure/openapi_sax_parsers.hpp

        src/handlers/insecure/openapi.cpp

        include/handlers/insecure/insecuresecretpost/handler.hpp
        include/handlers/insecure/chaotic_handlers_list.hpp
        include/handlers/insecure/insecuresecretpost/requests.hpp
        include/handlers/insecure/insecuresecretpost/responses.hpp

        src/handlers/insecure/insecuresecretpost/handler.cpp
        src/handlers/insecure/insecuresecretpost/requests.cpp
        src/handlers/insecure/insecuresecretpost/responses.cpp

        config.chaotic.yaml
)

RUN_PROGRAM(
    taxi/uservices/userver/scripts/chaotic
        ${BINDIR}/config.chaotic.yaml
        ${CURDIR}/../static_config.yaml
        -o ./config.yaml
    IN_NOPARSE
        ${BINDIR}/config.chaotic.yaml
        ../static_config.yaml
    OUT
        config.yaml
)

END()
