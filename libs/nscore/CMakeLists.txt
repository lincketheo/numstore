file(GLOB NSCORE_SRC CONFIGURE_DEPENDS *.c)
file(GLOB NSCORE_TESTING_SRC CONFIGURE_DEPENDS *.c)

if(WIN32)
  list(APPEND NSCORE_SRC ./intf/os_windows.c)
elseif(APPLE)
  list(APPEND NSCORE_SRC ./intf/os_darwin.c)
elseif(EMSCRIPTEN)
  list(APPEND NSCORE_SRC ./intf/os_emscripten.c)
else()
  list(APPEND NSCORE_SRC ./intf/os_posix.c)
endif()

add_ns_library(nscore
  SOURCES
    ${NSCORE_SRC}
  TEST_SOURCES
    ${NSCORE_TEST_SRC}
  EXTERNAL_LIBS
      backtrace
      pthread
)
