cmake_minimum_required(VERSION 3.20)
project(try1)

set(CMAKE_CXX_STANDARD 14)

add_executable(try1 main/main.cpp main/client.cpp main/client.h main/server.cpp main/server.h main/message.h main/message.cpp
        main/unifiedBlock.cpp main/unifiedBlock.h main/blockHead.cpp main/blockHead.h main/fileLayer.cpp main/fileLayer.h main/fileBlockHead.cpp main/fileBlockHead.h main/netBase.cpp main/netBase.h
        main/prettyPrint.h main/prettyPrint.cpp main/gbnSender.cpp main/gbnSender.h main/renoSender.cpp main/renoSender.h main/gbnRenoSender.cpp main/gbnRenoSender.h)
#add_executable(try1 test_block/unifiedBlock.cpp test_block/unifiedBlock.h test_block/blockHead.cpp test_block/blockHead.h test_block/main.cpp)
#add_executable(try1 test/rdt-part3.h test/test-client.c test/test-server.c)

target_link_libraries(try1 WS2_32.Lib winmm.lib)

set_target_properties(${TARGET} PROPERTIES
        COMPILE_FLAGS "-pthread"
        LINK_FLAGS "-pthread")
