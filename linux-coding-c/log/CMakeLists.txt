# At least this cmake version is needed
cmake_minimum_required (VERSION 3.8)

project(my_log)

set(PACKAGE_NAME my_log)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_BINARY_DIR}/)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-function")

set(SRCS)
file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*/*.c)

add_library(${PACKAGE_NAME} SHARED
    ${SRCS}
    )

# target_link_libraries(${PACKAGE_NAME}
#                 pthread rt m dl
#                 )
install(TARGETS ${PACKAGE_NAME}
	DESTINATION lib
)
