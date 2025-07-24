if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # The OS is supported, hooray!
elseif(CMAKE_SYSTEM_NAME MATCHES "BSD")
    message(WARNING "userver is known not to build under BSD. "
                    "It could be possible with some community help - maybe you could provide it?"
    )
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    message(FATAL_ERROR "userver cannot build natively on Windows. "
                        "Please use wsl or a VM. VSCode is known to provide good remote development experience."
    )
else()
    message(WARNING "An exotic OS detected: ${CMAKE_SYSTEM_NAME}. " "userver only supports Linux and macOS.")
endif()
