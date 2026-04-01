# At least this cmake version is needed
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
link_directories(${PROJECT_BINARY_DIR}/)

set(SRCS)
file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.c)

add_library(${PACKAGE_NAME}
    ${SRCS}
    )

target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/lib")
target_link_libraries(${PACKAGE_NAME}
                pthread gio-2.0 gobject-2.0 gmodule-2.0 gthread-2.0 rt glib-2.0
                json-glib-1.0
                readline
                )

install(TARGETS ${PACKAGE_NAME}
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
