include(ExternalProject)

set(thrift_DESTDIR "${CMAKE_CURRENT_BINARY_DIR}/lib/thrift_install")

ExternalProject_Add(
	thrift
        GIT_REPOSITORY git@github.yandex-team.ru:taxi-external/thrift.git
        GIT_TAG 0.11.0
        TIMEOUT 10
        CMAKE_ARGS -DBUILD_TESTING=OFF
                -DBUILD_EXAMPLES=OFF
                -DBUILD_TUTORIALS=OFF
                -DWITH_JAVA=OFF
                -DWITH_PYTHON=OFF
                -DWITH_HASKELL=OFF
                -DWITH_SHARED_LIB=OFF
                -DWITH_QT4=OFF
                -DWITH_QT5=OFF
                -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
                -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
        INSTALL_COMMAND make DESTDIR=${thrift_DESTDIR} install
        UPDATE_COMMAND ""
        # Wrap download, configure and build steps in a script to log output
        LOG_DOWNLOAD ON
        LOG_CONFIGURE OFF
        LOG_BUILD ON)

set(THRIFT_INCLUDE_DIR ${thrift_DESTDIR}/usr/local/include)
file(MAKE_DIRECTORY ${THRIFT_INCLUDE_DIR})
list(APPEND CMAKE_PREFIX_PATH "${thrift_DESTDIR}/usr/local")
