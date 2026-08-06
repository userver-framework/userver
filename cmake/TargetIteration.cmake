# Functions for traversing the CMake target link graph.
#
# Provides:
#
# * _userver_collect_linked_targets function that collects all targets a given target
#   links against, directly or transitively.
#
# Implementation note: public functions here should be usable even without a direct
# include of this script, so the functions should not rely on non-cache variables
# being present.
include_guard(GLOBAL)

# Collects all CMake targets that `target` links against, directly or transitively,
# into `output_var` (deduplicated). There is no built-in configure-time query for
# transitive dependencies, so the link graph is walked iteratively over
# LINK_LIBRARIES / INTERFACE_LINK_LIBRARIES. Link entries that are not plain targets
# (raw libraries, generator expressions, etc.) are ignored without inspection.
#
# Membership is tracked via per-target marker variables (O(1) lookup) instead of a
# list scan, so the traversal is linear in the number of targets and edges and is
# immune to cycles and diamond-dependency blowup. The markers are plain function-local
# variables: they are visible across the whole loop, are cleaned up automatically when
# the function returns, and never leak between separate invocations.
function(_userver_collect_linked_targets target output_var)
    set(visited "")
    set(frontier "${target}")

    while(frontier)
        set(next_frontier "")
        foreach(current IN LISTS frontier)
            if(NOT TARGET "${current}")
                continue()
            endif()

            # Resolve aliases (e.g. userver::postgresql -> userver-postgresql).
            get_target_property(aliased_target "${current}" ALIASED_TARGET)
            if(aliased_target)
                set(current "${aliased_target}")
            endif()

            string(MAKE_C_IDENTIFIER "userver_clt_seen_${current}" seen_marker)
            if(DEFINED ${seen_marker})
                continue()
            endif()
            set(${seen_marker} TRUE)
            list(APPEND visited "${current}")

            get_target_property(target_type "${current}" TYPE)
            # LINK_LIBRARIES may not be read on INTERFACE libraries on old CMake.
            if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
                get_target_property(link_libraries "${current}" LINK_LIBRARIES)
                if(link_libraries)
                    list(APPEND next_frontier ${link_libraries})
                endif()
            endif()
            get_target_property(interface_libraries "${current}" INTERFACE_LINK_LIBRARIES)
            if(interface_libraries)
                list(APPEND next_frontier ${interface_libraries})
            endif()
        endforeach()
        set(frontier "${next_frontier}")
    endwhile()

    set(${output_var}
        "${visited}"
        PARENT_SCOPE
    )
endfunction()
