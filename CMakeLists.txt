######################## common settings #############################################
cmake_minimum_required(VERSION 2.8)             #确定版本号，必须步骤
project(DNN_with_SGX)                           #项目名称，随便写
set(SGX_SDK /data/lz/sgxsdk)                  #SDK的安装路径
set(SGX_MODE SIM)                                #参数的设置，注意，如果给变量赋值的时候，值本身有空格，那么值需要使用“”来保证赋值的正确性，否则赋值可能存在错误
set(SGX_ARCH x64)
set(SGX_DEBUG 1)
set(SGX_COMMON_FLAGS -m64)
set(SGX_LIBRARY_PATH ${SGX_SDK}/lib64)
set(SGX_EDGER8R ${SGX_SDK}/bin/x64/sgx_edger8r)
set(SGX_ENCLAVE_SIGNER ${SGX_SDK}/bin/x64/sgx_sign) #以上的参数都是根据自己的机器设置，注意不要设置错误

set(OPENMP 1)
set(SGX_DNNL 0)
set(SHARED 0)
#####################################################################################
if(SGX_DEBUG EQUAL 1)
   message("Building in debug mode")
   set(SGX_COMMON_FLAGS "${SGX_COMMON_FLAGS} -O0 -g")
   add_definitions(-D DEBUG)
else()
   set(SGX_COMMON_FLAGS "${SGX_COMMON_FLAGS} -Ofast")
endif(SGX_DEBUG EQUAL 1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SGX_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${SGX_COMMON_FLAGS}")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)

set(EDL_FILES_PATH ${PROJECT_SOURCE_DIR}/Enclave/edls)


find_package(OpenSSL 1.1.0 REQUIRED)
if(OpenSSL_FOUND)
   message("OpenSSL FOUND")
endif()

find_package(OpenMP REQUIRED)
if(OpenMP_FOUND)
   message("OPENMP FOUND")
endif()

add_subdirectory(App)                           #执行App文件夹中的CMakeLists.txt
add_subdirectory(Enclave)                       #执行Enclave文件夹中的CMakeLists.txt
# add_subdirectory(client)
# add_subdirectory(server)


