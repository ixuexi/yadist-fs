project(thinmsg)

find_package(ZeroMQ)
find_package(czmq)

add_definitions(-DZMQ_BUILD_DRAFT_API)
add_definitions(-DCZMQ_BUILD_DRAFT_API)

add_executable(mser mser.c)
target_link_libraries(mser PRIVATE libzmq czmq)

add_executable(mcli mcli.c)
target_link_libraries(mcli PRIVATE libzmq czmq)

add_executable(minproc minproc.c)
target_link_libraries(minproc PRIVATE libzmq czmq)

install(TARGETS mser mcli minproc
        RUNTIME DESTINATION bin
)
