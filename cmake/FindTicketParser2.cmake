find_path(TICKET_PARSER2_INCLUDE_DIRECTORIES "ticket_parser2/ticket_parser.h")
find_library(TICKET_PARSER2_LIBRARIES ticket_parser2)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TicketParser2
    "Can't find libticket-parser2, please install libticket-parser2-dev or brew install ticket_parser2"
    TICKET_PARSER2_INCLUDE_DIRECTORIES TICKET_PARSER2_LIBRARIES)

if (TicketParser2_FOUND)
    if (NOT TARGET TicketParser2)
        add_library(TicketParser2 INTERFACE IMPORTED)
    endif()
    set_property(TARGET TicketParser2 PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES "${TICKET_PARSER2_INCLUDE_DIRECTORIES}")
    set_property(TARGET TicketParser2 PROPERTY
        INTERFACE_LINK_LIBRARIES "${TICKET_PARSER2_LIBRARIES}")
endif()

mark_as_advanced(TICKET_PARSER2_INCLUDE_DIRECTORIES TICKET_PARSER2_LIBRARIES)
