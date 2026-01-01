# Function to create a Python extension module
# Usage:
#   add_python_module(
#     TARGET target_name
#     MODULE_NAME python_module_name
#     SOURCES source1.c source2.c ...
#     LIBRARY_DEPENDENCY library_to_link
#     OUTPUT_DIR output_directory
#   )
function(add_python_module)
    set(options "")
    set(oneValueArgs TARGET MODULE_NAME LIBRARY_DEPENDENCY OUTPUT_DIR)
    set(multiValueArgs SOURCES)

    cmake_parse_arguments(ARG
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN})

    # Check if Python3 is available
    find_package(Python3 COMPONENTS Interpreter Development)

    if(NOT Python3_FOUND)
        message(STATUS "Python3 not found, skipping ${ARG_TARGET}")
        return()
    endif()

    # Create the Python extension module
    add_library(${ARG_TARGET} MODULE ${ARG_SOURCES})

    # Set properties for Python modules
    set_target_properties(${ARG_TARGET} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${ARG_MODULE_NAME}"
        SUFFIX ".${Python3_SOABI}${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )

    # Set output directory if specified
    if(ARG_OUTPUT_DIR)
        set_target_properties(${ARG_TARGET} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${ARG_OUTPUT_DIR}"
        )
    endif()

    # Include Python headers
    target_include_directories(${ARG_TARGET} PRIVATE
        ${Python3_INCLUDE_DIRS}
    )

    # Link against Python libraries
    target_link_libraries(${ARG_TARGET} PRIVATE
        ${Python3_LIBRARIES}
    )

    # Link against the specified library dependency
    if(ARG_LIBRARY_DEPENDENCY)
        target_link_libraries(${ARG_TARGET} PRIVATE
            ${ARG_LIBRARY_DEPENDENCY}
        )
    endif()

    # Set C standard
    set_property(TARGET ${ARG_TARGET} PROPERTY C_STANDARD 11)

    # Compiler flags
    target_compile_options(${ARG_TARGET} PRIVATE
        -Wall
        -Wextra
    )

    message(STATUS "Python module ${ARG_MODULE_NAME} enabled (target: ${ARG_TARGET})")
endfunction()
