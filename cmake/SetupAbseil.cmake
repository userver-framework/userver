# userver does not use Abseil directly, but some libraries need it.

include(DownloadUsingCPM)

CPMAddPackage(
    NAME abseil-cpp
    VERSION 20230802.1
    GIT_TAG 20230802.1
    GITHUB_REPOSITORY abseil/abseil-cpp
    SYSTEM
    OPTIONS
    "ABSL_PROPAGATE_CXX_STD ON"
    "ABSL_ENABLE_INSTALL ON"
)

mark_targets_as_system("${abseil-cpp_SOURCE_DIR}")
write_package_stub(absl)
