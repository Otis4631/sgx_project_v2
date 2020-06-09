

if(${SGX_MODE} EQUAL "HW")
    set(Urts_Library_Name sgx_urts)
    set(UAE_Server_Name sgx_uae_service)
ELSE()
    set(Urts_Library_Name sgx_urts_sim)
    set(UAE_Server_Name sgx_uae_service_sim)
endif()

set(LIBRARY ${Urts_Library_Name} pthread ${UAE_Server_Name})
if(${OPENMP_FOUND})
    set(LIBRARY ${LIBRARY} ${OPENMP_LIBRARIES})
endif()



set(APP_DIR ${PROJECT_SOURCE_DIR}/App)
set(UNTRUSTED_BRIDGE ${APP_DIR}/src/Enclave_u.c ${APP_DIR}/include/Enclave_u.h)

add_custom_command(OUTPUT ${UNTRUSTED_BRIDGE} PRE_BUILD
    COMMAND cd ${APP_DIR}/src && ${SGX_EDGER8R} --untrusted ${EDL_FILES_PATH}/Enclave.edl --search-path ${EDL_FILES_PATH} --search-path ${SGX_SDK}/include && mv Enclave_u.h ../include/
    COMMENT "make ucode"
)

# ADD_CUSTOM_APP(
#     untrusted_bridge
#    DEPENDS ${UNTRUSTED_BRIDGE}
# )

file(GLOB_RECURSE App_Cpp_Files "*.cpp")   #将App文件夹中的cpp文件列表赋值给APP_CPP_FILES
file(GLOB_RECURSE App_C_Files "*.c")   #将App文件夹中的cpp文件列表赋值给APP_CPP_FILES

include_directories(${PROJECT_SOURCE_DIR}/common ${APP_DIR}/include ${SGX_SDK}/include)  #包含所需要的头文件，后面记得链接,相当于Makefile文件中“-L”选项
set(App_C_Flags " -fPIC -Wno-attributescmake")#设置编译App文件夹中C文件的文件选项
if(OPENMP EQUAL 1)
    set(App_C_Flags "${App_C_Flags} -fopenmp")
endif()


set(App_Cpp_Flags ${App_C_Flags})                         #设置编译App文件夹中的CPP文件编译选项行
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${App_Cpp_Flags}")         #将CMake中编译CPP文件中的编译选项设置成我们自己的选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${App_C_Flags}")            #将将CMake中编译C文件中的编译选项设置成我们自己的选项
# set(CMAKE_EXE_LINKER_FLAGS "-L/usr/local/lib -lcrypto  -lpthread  -L${SGX_LIBRARY_PATH} -l${Urts_Library_Name}")   
set(CMAKE_CXX_STANDARD 11)   #设置C++的std，相当于makefile文件中的-std=c++11

# target_include_directories(${SGX_LIBRARY_PATH})
 LINK_DIRECTORIES(${SGX_LIBRARY_PATH})    #注意，不可信部分链接的库不再${SDK}/lib64中，而是在/usr/lib中，通过本条指令，这样在链接的时候，我们使用的库Cmake就会在/usr/lib中寻找

# set(current_dir ${CMAKE_CURRENT_SOURCE_DIR})

# set(CMAKE_INSTALL_PREFIX )

if(${SHARED} EQUAL 1)
    add_library(${APP} SHARED ${App_Cpp_Files} ${App_C_Files} ${UNTRUSTED_BRIDGE})
else()
    add_executable(${APP} ${App_Cpp_Files} ${App_C_Files} ${UNTRUSTED_BRIDGE})
endif()

TARGET_LINK_LIBRARIES(${APP} PRIVATE ${LIBRARY}) 