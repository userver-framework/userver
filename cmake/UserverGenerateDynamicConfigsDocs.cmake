function(_userver_add_target_gen_dynamic_configs_docs)
    file(GLOB YAML_FILENAMES */dynamic_configs/*.yaml)

    add_custom_target(
        userver-gen-dynamic-configs-docs
        COMMENT "Generate dynamic_configs .md docs"
	COMMAND 
	    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/docs/dynamic_config_yaml_to_md.py
	    -o ${CMAKE_CURRENT_BINARY_DIR}/docs-dynamic-configs
	    ${YAML_FILENAMES}
    )
endfunction()
