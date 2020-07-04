project (dist)

include_directories(include)

add_subdirectory(msg)
add_subdirectory(fuse)

add_executable(store_ser store_ser.c)
add_executable(store_act store_act.c)

target_link_libraries(store_ser PRIVATE msg passthrough)
target_link_libraries(store_act PRIVATE msg passthrough)

configure_file(env.sh.in ${CMAKE_CURRENT_BINARY_DIR}/env.sh)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/env.sh
        DESTINATION bin
)

install(TARGETS store_ser store_act
        RUNTIME DESTINATION bin
)

install(PROGRAMS start_ser.sh start_act.sh chroot_test.sh
        DESTINATION bin
)

install(FILES yadist.conf
        DESTINATION conf
)

