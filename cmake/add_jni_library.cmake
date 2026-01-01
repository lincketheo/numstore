# Function to create a JNI shared library
# Usage:
#   add_jni_library(
#     TARGET target_name
#     MODULE_NAME jni_library_name
#     SOURCES source1.c source2.c ...
#     LIBRARY_DEPENDENCY library_to_link
#     OUTPUT_DIR output_directory
#   )
function(add_jni_library)
    set(options "")
    set(oneValueArgs TARGET MODULE_NAME LIBRARY_DEPENDENCY OUTPUT_DIR)
    set(multiValueArgs SOURCES)

    cmake_parse_arguments(ARG
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN})

    # Check if Java and JNI are available
    find_package(JNI)
    find_package(Java COMPONENTS Development)

    if(NOT JNI_FOUND)
        message(STATUS "JNI not found, skipping ${ARG_TARGET}")
        return()
    endif()

    # Create the JNI shared library
    add_library(${ARG_TARGET} SHARED ${ARG_SOURCES})

    # Set properties for JNI libraries
    set_target_properties(${ARG_TARGET} PROPERTIES
        PREFIX "lib"
        OUTPUT_NAME "${ARG_MODULE_NAME}"
        SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )

    # Set output directory if specified
    if(ARG_OUTPUT_DIR)
        set_target_properties(${ARG_TARGET} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${ARG_OUTPUT_DIR}"
        )
    endif()

    # Include JNI headers
    target_include_directories(${ARG_TARGET} PRIVATE
        ${JNI_INCLUDE_DIRS}
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
    # JNI functions don't need prototypes/declarations, so disable those warnings
    target_compile_options(${ARG_TARGET} PRIVATE
        -Wall
        -Wextra
        -Wno-missing-prototypes
        -Wno-missing-declarations
        -Wno-unused-parameter
    )

    message(STATUS "JNI library ${ARG_MODULE_NAME} enabled (target: ${ARG_TARGET})")
endfunction()
