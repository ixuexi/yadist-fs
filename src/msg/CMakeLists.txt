
find_package(ZeroMQ)
find_package(czmq)

add_library(mq_draft INTERFACE)
target_compile_definitions(mq_draft INTERFACE -DZMQ_BUILD_DRAFT_API -DCZMQ_BUILD_DRAFT_API)

#add_executable(server server.c)
#target_link_libraries(server PRIVATE libzmq czmq)

#add_executable(client client.c)
#target_link_libraries(client PRIVATE libzmq czmq)

add_library(msg SHARED client.c server.c fs.c hash.c config.c cmd.c)
target_link_libraries(msg PRIVATE libzmq czmq $<BUILD_INTERFACE:mq_draft>)

add_executable(yasched sched.c)
target_link_libraries(yasched PRIVATE libzmq czmq $<BUILD_INTERFACE:mq_draft>)

add_executable(yaswitch switch.c)
target_link_libraries(yaswitch PRIVATE libzmq czmq $<BUILD_INTERFACE:mq_draft>)

install(TARGETS msg yasched yaswitch
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)
