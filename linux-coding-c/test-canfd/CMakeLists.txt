CMAKE_MINIMUM_REQUIRED (VERSION 3.8)

project(canfd_test)

set(PACKAGE_NAME canfd_test)

# The library has to be dynamically linked to the header
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-function")

set(SRCS)
file(GLOB SRCS  ${CMAKE_CURRENT_SOURCE_DIR}/*.c
                ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
                )

add_executable(${PACKAGE_NAME} ${SRCS})

link_directories(${PROJECT_BINARY_DIR}/)

install(TARGETS canfd_test
	DESTINATION bin
)
