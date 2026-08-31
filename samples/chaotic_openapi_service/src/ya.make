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
    auth_bearer.cpp
    auth_digest.cpp
    hello_handler.cpp
    say_hello.cpp
    handlers/insecure/callsecretget/view.cpp
    handlers/insecure/insecuresecretpost/view.cpp
    handlers/secure/greetingget/view.cpp
    handlers/secure/secretget/view.cpp
)

ADDINCL(
    GLOBAL ${ARCADIA_BUILD_ROOT}/${MODDIR}/include
    GLOBAL ${ARCADIA_BUILD_ROOT}/${MODDIR}/secure-gen/include
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
        --name secure
        --gen client
        -o ${BINDIR}/
        --clang-format ''
        ../clients/secure.yaml
    OUTPUT_INCLUDES
        ${CHAOTIC_INCLUDES}
        ${CHAOTIC_OPENAPI_INCLUDES}
    IN_NOPARSE
        ../clients/secure.yaml
    OUT
        src/clients/secure/client_impl.cpp
        src/clients/secure/client.cpp
        src/clients/secure/requests.cpp
        src/clients/secure/responses.cpp
        src/clients/secure/component.cpp
        src/clients/secure/exceptions.cpp

        include/clients/secure/client_impl.hpp
        include/clients/secure/client.hpp
        include/clients/secure/requests.hpp
        include/clients/secure/responses.hpp
        include/clients/secure/exceptions.hpp
        include/clients/secure/component.hpp
        include/clients/secure/qos.hpp

        src/clients/secure/secure.cpp
        include/clients/secure/secure.hpp
        include/clients/secure/secure_fwd.hpp
        include/clients/secure/secure_parsers.ipp
        include/clients/secure/secure_sax_parsers.hpp
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
        handlers/insecure/callsecretget/view.hpp
        handlers/insecure/insecuresecretpost/view.hpp
    IN_NOPARSE
        ../handlers/insecure/openapi.yaml
    OUT
        include/handlers/insecure/openapi.hpp
        include/handlers/insecure/openapi_fwd.hpp
        include/handlers/insecure/openapi_parsers.ipp
        include/handlers/insecure/openapi_sax_parsers.hpp

        src/handlers/insecure/openapi.cpp

        include/handlers/insecure/callsecretget/handler.hpp
        include/handlers/insecure/insecuresecretpost/handler.hpp
        include/handlers/insecure/chaotic_handlers_list.hpp
        include/handlers/insecure/callsecretget/requests.hpp
        include/handlers/insecure/callsecretget/responses.hpp
        include/handlers/insecure/insecuresecretpost/requests.hpp
        include/handlers/insecure/insecuresecretpost/responses.hpp

        src/handlers/insecure/callsecretget/handler.cpp
        src/handlers/insecure/callsecretget/requests.cpp
        src/handlers/insecure/callsecretget/responses.cpp
        src/handlers/insecure/insecuresecretpost/handler.cpp
        src/handlers/insecure/insecuresecretpost/requests.cpp
        src/handlers/insecure/insecuresecretpost/responses.cpp
    OUT_NOAUTO
        config.chaotic.yaml
)

RUN_PROGRAM(
    taxi/uservices/userver/chaotic-openapi/bin
        --name secure
        --gen handlers
        -o ${BINDIR}/secure-gen
        --clang-format ''
        ../handlers/secure/openapi.yaml
    OUTPUT_INCLUDES
        ${CHAOTIC_INCLUDES}
        ${CHAOTIC_OPENAPI_INCLUDES}
        handlers/secure/greetingget/view.hpp
        handlers/secure/secretget/view.hpp
    IN_NOPARSE
        ../handlers/secure/openapi.yaml
    OUT
        secure-gen/include/handlers/secure/openapi.hpp
        secure-gen/include/handlers/secure/openapi_fwd.hpp
        secure-gen/include/handlers/secure/openapi_parsers.ipp
        secure-gen/include/handlers/secure/openapi_sax_parsers.hpp

        secure-gen/src/handlers/secure/openapi.cpp

        secure-gen/include/handlers/secure/greetingget/handler.hpp
        secure-gen/include/handlers/secure/secretget/handler.hpp
        secure-gen/include/handlers/secure/chaotic_handlers_list.hpp
        secure-gen/include/handlers/secure/greetingget/requests.hpp
        secure-gen/include/handlers/secure/greetingget/responses.hpp
        secure-gen/include/handlers/secure/secretget/requests.hpp
        secure-gen/include/handlers/secure/secretget/responses.hpp

        secure-gen/src/handlers/secure/greetingget/handler.cpp
        secure-gen/src/handlers/secure/greetingget/requests.cpp
        secure-gen/src/handlers/secure/greetingget/responses.cpp
        secure-gen/src/handlers/secure/secretget/handler.cpp
        secure-gen/src/handlers/secure/secretget/requests.cpp
        secure-gen/src/handlers/secure/secretget/responses.cpp
    OUT_NOAUTO
        secure-gen/config.chaotic.yaml
)

END()
