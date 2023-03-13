cmake_minimum_required(VERSION 3.20)
project(proj1)

set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_C_STANDARD 14)

#add_executable(proj1 test/main.cpp test/server.cpp test/client.cpp test/client.h test/server.h)
#add_executable(proj1 test2/queue_block.h test2/main.cpp)
add_executable(proj1 main/main.cpp main/client.cpp main/client.h main/server.cpp main/server.h main/queue_block.h main/message.h main/bean.h main/prettyPrint.cpp main/prettyPrint.h)

target_link_libraries(proj1 WS2_32.Lib)

set_target_properties(${TARGET} PROPERTIES
        COMPILE_FLAGS "-pthread"
        LINK_FLAGS "-pthread")