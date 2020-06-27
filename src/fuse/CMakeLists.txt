project(simple)
cmake_minimum_required(VERSION 3.16)
#set(FUSE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../package/usr/local)

add_library(libfuse INTERFACE)
target_include_directories(libfuse INTERFACE ${FUSE_PATH}/include/fuse3)
target_link_directories(libfuse INTERFACE ${FUSE_PATH}/lib/x86_64-linux-gnu)
target_link_libraries(libfuse INTERFACE -lfuse3)
#target_link_options(libfuse INTERFACE -Wl,--verbose)

#add_executable(passthrough passthrough.c)
add_library(passthrough SHARED passthrough.c)
target_link_libraries(passthrough PRIVATE $<BUILD_INTERFACE:libfuse>)

install(TARGETS passthrough
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
)
