set(TARGET libapp.so)

if(${SGX_MODE} EQUAL "HW")
    set(Urts_Library_Name sgx_urts)
    set(UAE_Server_Name sgx_uae_service)
ELSE()
    set(Urts_Library_Name sgx_urts_sim)
    set(UAE_Server_Name sgx_uae_service_sim)
endif()


set(UNTRUSTED_BRIDGE ${PROJECT_SOURCE_DIR}/src/Enclave_u.c ${PROJECT_SOURCE_DIR}/include/Enclave_u.h)
set(EDL_FILES_PATH ${PROJECT_SOURCE_DIR}/Enclave/edls)
set(APP_DIR ${PROJECT_SOURCE_DIR}/App)

add_custom_command(OUTPUT ${UNTRUSTED_BRIDGE} PRE_BUILD
    COMMAND ${SGX_EDGER8R} --untrusted ${EDL_FILES_PATH}/Enclave.edl --search-path ${EDL_FILES_PATH} --search-path ${SGX_SDK}/include
   COMMENT "make ucode"
)

ADD_CUSTOM_TARGET(
    untrusted_bridge ALL
   DEPENDS ${UNTRUSTED_BRIDGE}
)


FILE(GLOB_RECURSE App_Cpp_Files "*.cpp")   #将App文件夹中的cpp文件列表赋值给APP_CPP_FILES
FILE(GLOB_RECURSE App_C_Files "*.c")   #将App文件夹中的cpp文件列表赋值给APP_CPP_FILES

MESSAGE( STATUS ${APP_DIR}/include)

include_directories(${APP_DIR}/include ${SGX_SDK}/include)  #包含所需要的头文件，后面记得链接,相当于Makefile文件中“-L”选项
set(App_C_Flags "${SGX_COMMON_CFLAGS} -fPIC -Wno-attributescmake")#设置编译App文件夹中C文件的文件选项
set(App_C_Flags "${App_C_Flags} -DDEBUG -UNDEBUG -UEDEBUG")   #设置编译App文件夹中C文件的文件选项
set(App_Cpp_Flags "${App_C_Flags}")                         #设置编译App文件夹中的CPP文件编译选项行
set(App_Link_Flags "${SGX_COMMON_CFLAGS} -L${SGX_LIBRARY_PATH} -L/usr/lib -l${Urts_Library_Name} -lpthread -l${UAE_Server_Name}")    #设置生成应用程序的链接选项，因为要用到SDK中带有的库
set(CMAKE_CXX_FLAGS ${App_Cpp_Flags})         #将CMake中编译CPP文件中的编译选项设置成我们自己的选项
set(CMAKE_C_FLAGS ${App_C_Flags})            #将将CMake中编译C文件中的编译选项设置成我们自己的选项
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${App_Link_Flags}")#设置链接生成可执行文件的时候的链接选项
set(CMAKE_CXX_STANDARD 11)   #设置C++的std，相当于makefile文件中的-std=c++11
LINK_DIRECTORIES("/usr/lib")    #注意，不可信部分链接的库不再${SDK}/lib64中，而是在/usr/lib中，通过本条指令，这样在链接的时候，我们使用的库Cmake就会在/usr/lib中寻找


add_library(${TARGET} ${App_Cpp_Files} ${App_C_Files} ${UNTRUSTED_BRIDGE})
# ADD_EXECUTABLE(${TARGET} ${App_Cpp_Files} ${UNTRUSTED_BRIDGE})#生成可执行文件
TARGET_LINK_LIBRARIES(${TARGET} ${Urts_Library_Name} pthread ${UAE_Server_Name} ${OPENSSL_LIBRARIES}) #可执行文件需要的库，注意链接库的先后顺序，有的库需要依赖前面的库函数，如果顺序错误，可能会报错，说找不到相应函数
