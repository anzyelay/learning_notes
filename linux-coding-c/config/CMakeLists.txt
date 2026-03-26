cmake_minimum_required (VERSION 3.8)
project (demo-config)

set(SRCS main.c config.c)
add_executable(demo-config ${SRCS})
target_link_libraries(demo-config
                pthread gio-2.0 gobject-2.0 gmodule-2.0 gthread-2.0 rt glib-2.0
                json-glib-1.0
                readline
                )
install(TARGETS demo-config
	DESTINATION bin
)
