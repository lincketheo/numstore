# Generate shell parser
add_lemon_parser(
  PREFIX shell
  GRAMMAR ${CMAKE_CURRENT_SOURCE_DIR}/parsers/shell_parser.y
  OUTPUT_VAR SHELL_PARSER_C
)

# Generate net parser
add_lemon_parser(
  PREFIX net
  GRAMMAR ${CMAKE_CURRENT_SOURCE_DIR}/parsers/net_parser.y
  OUTPUT_VAR NET_PARSER_C
)

# Collect all compiler source files
file(GLOB COMPILER_SRC CONFIGURE_DEPENDS *.c)
file(GLOB NETWORK_SRC CONFIGURE_DEPENDS network/*.c)

# Add generated parsers to sources
list(APPEND COMPILER_SRC ${SHELL_PARSER_C} ${NET_PARSER_C})
list(APPEND COMPILER_SRC ${NETWORK_SRC})

add_ns_library(nscompiler
  SOURCES
    ${COMPILER_SRC}
  DEPENDENCIES
    ${LIBS}/nstypes
)

# Add dependencies on parser generation targets
add_dependencies(nscompiler shell_parser_gen net_parser_gen)
