function(add_ns_executable TARGET)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs 
        SOURCES 
        DEPENDENCIES 
        EXTERNAL_LIBS 
        EXTERNAL_INCLUDES)

    cmake_parse_arguments(ARG 
        "${options}" 
        "${oneValueArgs}" 
        "${multiValueArgs}" 
         ${ARGN})

    # Create executable
    add_executable(${TARGET} ${ARG_SOURCES})

    # Build include directories and link libraries from dependencies
    set(INCLUDE_DIRS "")
    set(DEP_TARGETS "")
    foreach(DEP ${ARG_DEPENDENCIES})
        string(REGEX REPLACE ".*/([^/]+)$" "\\1" DEP_TARGET "${DEP}")
        list(APPEND INCLUDE_DIRS ${DEP}/include)
        list(APPEND DEP_TARGETS ${DEP_TARGET})
    endforeach()

    # Add external includes
    if(ARG_EXTERNAL_INCLUDES)
        list(APPEND INCLUDE_DIRS ${ARG_EXTERNAL_INCLUDES})
    endif()

    # Set includes and link libraries
    if(INCLUDE_DIRS)
        target_include_directories(${TARGET} PRIVATE ${INCLUDE_DIRS})
    endif()
    if(DEP_TARGETS)
        target_link_libraries(${TARGET} PRIVATE ${DEP_TARGETS})
    endif()
    if(ARG_EXTERNAL_LIBS)
        target_link_libraries(${TARGET} PRIVATE ${ARG_EXTERNAL_LIBS})
    endif()
endfunction()
