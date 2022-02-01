set(COMPILER_VERSION "0")
if (CMAKE_CXX_COMPILER MATCHES "^.*-([0-9]+)$")
    set(COMPILER_VERSION "${CMAKE_MATCH_1}")
endif()

# ar from binutils on old Ubuntu fails to link -flto=thin with the following message:
# ../../src/libyandex-taxi-userver.a: error adding symbols: Archive has no index; run ranlib to add one
# In Debian/Ubuntu llvm-ar is installed with version suffix.
# In Mac OS with brew llvm-ar is installed without version suffix
# into /usr/local/opt/llvm/ with brew
if (MACOS)
    set(LLVM_PATH_HINTS /usr/local/opt/llvm/bin)
endif()
find_program(LLVM_AR NAMES llvm-ar-${COMPILER_VERSION} llvm-ar HINTS ${LLVM_PATH_HINTS})
find_program(LLVM_RANLIB NAMES llvm-ranlib-${COMPILER_VERSION} llvm-ranlib HINTS ${LLVM_PATH_HINTS})
if (NOT LLVM_AR)
    set(LTO OFF)
    set(LTO_DISABLE_REASON "llvm-ar (from llvm-${COMPILER_VERSION}) not found")
elseif(NOT LLVM_RANLIB)
    set(LTO OFF)
    set(LTO_DISABLE_REASON "llvm-ranlib (from llvm-${COMPILER_VERSION}) not found")
else()
    set(CMAKE_AR ${LLVM_AR})
    set(CMAKE_RANLIB ${LLVM_RANLIB})
endif()
