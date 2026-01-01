# Function to generate parser from Lemon grammar file
# Usage: add_lemon_parser(
#   PREFIX <prefix>           # Prefix for parser (e.g., "shell", "net")
#   GRAMMAR <grammar_file>    # Path to .y grammar file
#   OUTPUT_VAR <var_name>     # Variable to store generated parser.c path
# )
function(add_lemon_parser)
    set(options "")
    set(oneValueArgs PREFIX GRAMMAR OUTPUT_VAR)
    set(multiValueArgs "")

    cmake_parse_arguments(ARG
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
         ${ARGN})

    if(NOT ARG_PREFIX)
        message(FATAL_ERROR "add_lemon_parser: PREFIX argument is required")
    endif()

    if(NOT ARG_GRAMMAR)
        message(FATAL_ERROR "add_lemon_parser: GRAMMAR argument is required")
    endif()

    if(NOT ARG_OUTPUT_VAR)
        message(FATAL_ERROR "add_lemon_parser: OUTPUT_VAR argument is required")
    endif()

    # Ensure lemon executable is built (only once)
    if(NOT TARGET lemon)
        add_executable(lemon ${CMAKE_SOURCE_DIR}/thirdparty/lemon/lemon.c)
        set_source_files_properties(
            ${CMAKE_SOURCE_DIR}/thirdparty/lemon/lemon.c
            PROPERTIES COMPILE_FLAGS "-w -O0")
    endif()

    # Set output paths
    set(PARSER_C ${CMAKE_CURRENT_BINARY_DIR}/${ARG_PREFIX}_parser.c)
    set(PARSER_H ${CMAKE_CURRENT_BINARY_DIR}/${ARG_PREFIX}_parser.h)

    # Generate parser from grammar
    add_custom_command(
        OUTPUT ${PARSER_C}

        COMMAND ${CMAKE_COMMAND} -E env sh -c
                "$<TARGET_FILE:lemon>  \
                   ${ARG_GRAMMAR} \
                 -T${CMAKE_SOURCE_DIR}/thirdparty/lemon/lempar.c \
                 -d${CMAKE_CURRENT_BINARY_DIR} \
                 -p${ARG_PREFIX}_parser || true"

        DEPENDS
            lemon
            ${ARG_GRAMMAR}
            ${CMAKE_SOURCE_DIR}/thirdparty/lemon/lempar.c

        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}

        COMMENT "Generating ${ARG_PREFIX}_parser.c / ${ARG_PREFIX}_parser.h with Lemon"

        VERBATIM
    )

    # Create a target for this parser generation
    add_custom_target(${ARG_PREFIX}_parser_gen DEPENDS ${PARSER_C})

    # Set compile flags for generated parser
    set_source_files_properties(${PARSER_C} PROPERTIES COMPILE_OPTIONS "-w;-O0")

    # Return the path to the generated parser.c
    set(${ARG_OUTPUT_VAR} ${PARSER_C} PARENT_SCOPE)
endfunction()
