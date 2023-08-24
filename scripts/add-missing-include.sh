#!/bin/bash

FILES_TO_PROCESS=''

before_pattern_insert_once() {
    local PATTERN=$1
    local INSERTION=$2
    sed -i -e "/${PATTERN}/{i \\${INSERTION}" -e ":p;n;bp}" ${FILES_TO_PROCESS}
}

update_files_to_process() {
    FILES_TO_PROCESS=`grep -L "<${HEADER}>" ${FILES_TO_PROCESS}`
    if [ -z "${FILES_TO_PROCESS}" ]; then
        echo "Done"
        exit 0
    fi
}

add_header_to_includes_with_prefix() {
    update_files_to_process
    echo "Adding ${HEADER} to includes with prefix ${1}"

    local PREFIX="${1}"
    before_pattern_insert_once "^#include <${PREFIX}" "#include <${HEADER}>"
}

add_header_before_first_namespace() {
    update_files_to_process
    echo "Adding ${HEADER} before namespace"
    before_pattern_insert_once "^namespace " "#include <${HEADER}>\n"
}

add_header_to_std_includes() {
    update_files_to_process
    echo "Adding ${HEADER} to std includes"
    before_pattern_insert_once "^#include <\([a-z]*\)>" "#include <${HEADER}>"
}

add_header_before_userver_includes() {
    update_files_to_process
    echo "Adding ${HEADER} before userver includes"
    before_pattern_insert_once "^#include <userver" "\n#include <${HEADER}>\n"
}

show_usage_and_exit() {
    local SCRIPT="$0"

    local FOLDER='services'
    local ENTITY='PeriodicTask '
    local HEADER='userver/utils/periodic_task.hpp'

    echo "Error: missing arguments!"
    echo "Example usage:"
    echo "    ENTITY='${ENTITY}' HEADER='${HEADER}' ${SCRIPT}"
    echo "Or:"
    echo "    FOLDER='${FOLDER}' ENTITY='${ENTITY}' HEADER='${HEADER}' ${SCRIPT}"
    exit 1
}

find_entity_files() {
    local FOLDER=${FOLDER:-$(pwd)}

    echo "Searching for '${ENTITY}' in '${FOLDER}' to add '${HEADER}' header"

    FILES_TO_PROCESS=`grep -lr "${ENTITY}" --include='*pp' ${FOLDER}`
}

main() {
    if [[ -z "${ENTITY}" ]] || [[ -z "${HEADER}" ]] ; then
        show_usage_and_exit
    fi

    find_entity_files

    if [[ $HEADER == userver/* ]]; then
        add_header_to_includes_with_prefix "userver"
    else
        if [[ $HEADER == fmt/* ]] ; then
            add_header_to_includes_with_prefix "fmt"
        elif [[ $HEADER == boost/* ]]; then
            add_header_to_includes_with_prefix "boost"
        else
            add_header_to_std_includes
        fi
        add_header_before_userver_includes
    fi

    add_header_before_first_namespace
}

main
