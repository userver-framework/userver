
function(check_submodule path filename)
    if (NOT EXISTS ${path}/${filename})
        message(FATAL_ERROR "Directory ${path} doesn't contain ${filename}. Please run 'git submodule update --init --recursive'.")
    endif()
endfunction()

