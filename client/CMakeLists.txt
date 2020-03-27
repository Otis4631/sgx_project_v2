

set(Target client.bin)

find_package(Boost REQUIRED COMPONENTS
    thread
    system
    program_options
    log

)
add_definitions(-DBOOST_LOG_DYN_LINK)

include_directories("include"  ${Boost_INCLUDE_DIRS})


file(GLOB_RECURSE Cpp_Files "*.cpp")

add_executable(${Target} ${Cpp_Files})
target_link_libraries(${Target} ${Boost_LIBRARIES})


