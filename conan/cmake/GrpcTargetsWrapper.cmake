# Wrapper around the original GrpcTarget script that defines userver-grpc target
# This target is required by the original GrpcTarget script but conan doesn't set it up
include(GrpcTargetsImpl)

if (NOT TARGET userver-grpc)
    add_library(userver-grpc INTERFACE)

    if (TARGET userver::userver) # cmake_find_package* generator
        target_link_libraries(userver-grpc INTERFACE userver::userver)
    elseif(TARGET CONAN_PKG::userver) # cmake generator
        target_link_libraries(userver-grpc INTERFACE CONAN_PKG::userver)
    endif()
endif()