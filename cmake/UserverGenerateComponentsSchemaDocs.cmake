function(_userver_add_target_gen_component_schemas_docs)
    file(GLOB_RECURSE YAML_FILENAMES */src/*.yaml */static_configs/*.yaml)

    add_custom_target(
        userver-gen-components-schema-docs
        COMMENT "Generate components schema .md docs"
        COMMAND ${USERVER_PYTHON_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/docs/components_schema_to_table.py -o
                ${CMAKE_CURRENT_BINARY_DIR}/components-schema ${YAML_FILENAMES}
    )
endfunction()
