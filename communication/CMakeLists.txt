

set(Client client.bin)
set(Server server.bin)

find_package(Boost REQUIRED COMPONENTS
    thread
    system
    program_options
    log

)
add_definitions(-DBOOST_LOG_DYN_LINK)

include_directories("include"  ${Boost_INCLUDE_DIRS})


file(GLOB_RECURSE Client_Cpp_Files "*.cpp")
list(FILTER Client_Cpp_Files EXCLUDE REGEX "server*")



file(GLOB_RECURSE Server_Cpp_Files "*.cpp")
list(FILTER Server_Cpp_Files EXCLUDE REGEX "client*")

add_executable(${Client} ${Client_Cpp_Files})
target_link_libraries(${Client} ${Boost_LIBRARIES})

add_executable(${Server} ${Server_Cpp_Files})
target_link_libraries(${Server} ${Boost_LIBRARIES})
