LIBRARY()

OWNER(
    g:taxi-common
)

CFLAGS(
    -DBOOST_CONTEXT_SOURCE
)

PEERDIR(
    contrib/restricted/boost/assert
    contrib/restricted/boost/config
    contrib/restricted/boost/core
    contrib/restricted/boost/mp11
    contrib/restricted/boost/pool
    contrib/restricted/boost/predef
    contrib/restricted/boost/smart_ptr
)

ADDINCL(
    taxi/fake-include/uboost_coro
    taxi/uservices/userver/uboost_coro/include
    taxi/uservices/userver/uboost_coro/src
)

SRCS(
    src/context/posix/stack_traits.cpp
)

IF (NOT SANITIZER_TYPE)
    IF (OS_DARWIN)
        IF (ARCH_X86_64)
            SRCS(
                src/context/asm/jump_x86_64_sysv_macho_gas.S
                src/context/asm/make_x86_64_sysv_macho_gas.S
                src/context/asm/ontop_x86_64_sysv_macho_gas.S
            )
        ELSE()
            SRCS(
                src/context/asm/jump_arm64_aapcs_macho_gas.S
                src/context/asm/make_arm64_aapcs_macho_gas.S
                src/context/asm/ontop_arm64_aapcs_macho_gas.S
            )
        ENDIF()
    ELSEIF (ARCH_I386)
        SRCS(
            src/context/asm/jump_i386_sysv_elf_gas.S
            src/context/asm/make_i386_sysv_elf_gas.S
            src/context/asm/ontop_i386_sysv_elf_gas.S
        )
    ELSEIF (ARCH_X86_64)
        SRCS(
            src/context/asm/jump_x86_64_sysv_elf_gas.S
            src/context/asm/make_x86_64_sysv_elf_gas.S
            src/context/asm/ontop_x86_64_sysv_elf_gas.S
        )
    ELSEIF (ARCH_ARM7)
        SRCS(
            src/context/asm/jump_arm_aapcs_elf_gas.S
            src/context/asm/make_arm_aapcs_elf_gas.S
            src/context/asm/ontop_arm_aapcs_elf_gas.S
        )
    ELSEIF (ARCH_ARM64)
        SRCS(
            src/context/asm/jump_arm64_aapcs_elf_gas.S
            src/context/asm/make_arm64_aapcs_elf_gas.S
            src/context/asm/ontop_arm64_aapcs_elf_gas.S
        )
    ENDIF()
ELSE()
    CFLAGS(
        GLOBAL -DBOOST_USE_UCONTEXT
    )

    IF (OS_DARWIN)
        CFLAGS(
            GLOBAL -Wno-error=deprecated-declarations
            GLOBAL -D_XOPEN_SOURCE=600
            GLOBAL -D_DARWIN_C_SOURCE
        )
    ENDIF()

    SRCS(
        src/context/continuation.cpp
        src/context/fiber.cpp
    )
ENDIF()

IF (SANITIZER_TYPE == "address")
    CFLAGS(
        GLOBAL -DBOOST_USE_ASAN
    )
ENDIF()


END()
