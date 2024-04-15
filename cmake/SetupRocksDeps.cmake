include_guard(GLOBAL)

if(userver_rocks_FOUND)
  return()
endif()

include(DownloadUsingCPM)

CPMAddPackage(
  NAME rocksdb
  GITHUB_REPOSITORY facebook/rocksdb
  GIT_TAG v8.11.3
)

set(userver_rocks_FOUND TRUE)
