project(zmq)

include(ExternalProject)

set(INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/package)

ExternalProject_Add(zeromq
                    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/zeromq
                    CMAKE_ARGS -DENABLE_DRAFTS=on -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}/zmq
#                    BUILD_ALWAYS TRUE
)

ExternalProject_Add(czmq
                    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/czmq
                    CMAKE_ARGS -DENABLE_DRAFTS=on -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}/czmq
                               -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=1 -DCMAKE_PREFIX_PATH=${INSTALL_DIR}/zmq
#                    BUILD_ALWAYS TRUE
                    DEPENDS zeromq
)

ExternalProject_Add(zstd
                    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/zstd/build/cmake
                    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}/zstd
#                    BUILD_ALWAYS TRUE
)

ExternalProject_Add(fuse
                    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/libfuse
                    CONFIGURE_COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/external/meson/meson.py <SOURCE_DIR>
                    BUILD_COMMAND ninja
                    INSTALL_COMMAND DESTDIR=${INSTALL_DIR}/fuse python3 ${CMAKE_CURRENT_SOURCE_DIR}/external/meson/meson.py install || echo "install done"
#                    BUILD_ALWAYS TRUE
)

set(PKG_PATH ${INSTALL_DIR}/zmq/share/cmake/ZeroMQ:${INSTALL_DIR}/czmq/share/cmake/czmq)
set(FUSE_PATH ${INSTALL_DIR}/fuse/usr/local)
ExternalProject_Add(dist
                    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src
                    CMAKE_ARGS -DCMAKE_PREFIX_PATH=${PKG_PATH} -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}/dist -DFUSE_PATH=${FUSE_PATH}
                    BUILD_ALWAYS TRUE
                    LIST_SEPARATOR :
                    DEPENDS zeromq czmq zstd fuse
)
