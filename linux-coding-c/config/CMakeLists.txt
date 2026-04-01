cmake_minimum_required (VERSION 3.8)

project(jsonconf)

set(PACKAGE_NAME jsonconf)
# add_compile_options(-Wall -Wextra -Werror)
# add_compile_options(-Wno-error=unused-parameter -Wno-incompatible-pointer-types)

find_package(PkgConfig REQUIRED)
pkg_search_module(GIO2 REQUIRED gio-unix-2.0)
if (GIO2_FOUND)
        INCLUDE_DIRECTORIES(${GIO2_INCLUDE_DIRS})
else()
        message("pkg: not found gio-unix-2.0")
endif()
pkg_search_module(JSON-GLIB REQUIRED json-glib-1.0)
if (JSON-GLIB_FOUND)
        INCLUDE_DIRECTORIES(${JSON-GLIB_INCLUDE_DIRS})
else()
        message("pkg: not found json-glib-1.0")
endif()

include_directories(${PROJECT_SOURCE_DIR}/)
include_directories(${PROJECT_SOURCE_DIR}/lib)
link_directories(${PROJECT_BINARY_DIR}/)

set(LIBSRCS)
file(GLOB LIBSRCS ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.c)

add_library(${PACKAGE_NAME}_shared SHARED
    ${LIBSRCS}
    )

add_library(${PACKAGE_NAME}_static
    ${LIBSRCS}
    )

set_target_properties(${PACKAGE_NAME}_shared PROPERTIES OUTPUT_NAME ${PACKAGE_NAME})
set_target_properties(${PACKAGE_NAME}_static PROPERTIES OUTPUT_NAME ${PACKAGE_NAME})
set_target_properties(${PACKAGE_NAME}_shared PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

target_include_directories(${PROJECT_NAME}_shared PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(${PROJECT_NAME}_static PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(${PACKAGE_NAME}_shared
                pthread gio-2.0 gobject-2.0 gmodule-2.0 gthread-2.0 rt glib-2.0
                json-glib-1.0
                readline
                )

install(TARGETS ${PACKAGE_NAME}_shared ${PACKAGE_NAME}_static
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION include
        )

install(FILES lib/jsonconfig.h
      DESTINATION include)

set(SRCS main.c)
add_executable(demo-config ${SRCS})
target_link_libraries(demo-config
                jsonconf
                )

install(TARGETS demo-config
	DESTINATION bin
)
