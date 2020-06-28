set(Trts_Library_Name sgx_trts)
set(Service_Library_Name sgx_tservice)

IF(${SGX_MODE} EQUAL 1)
    set(Trts_Library_Name sgx_trts)
    set(Service_Library_Name sgx_tservice)
ELSE()
    set(Trts_Library_Name sgx_trts_sim)
    set(Service_Library_Name sgx_tservice_sim)
ENDIF()

set(Signed_enclave_name enclave.signed.so)
set(Enclave_Name enclave)

set(ENCLAVE_DIR ${PROJECT_SOURCE_DIR}/Enclave)


set(TRUSTED_BRIDGE ${ENCLAVE_DIR}/src/Enclave_t.c ${ENCLAVE_DIR}/include/Enclave_t.h)

ADD_CUSTOM_COMMAND(OUTPUT ${TRUSTED_BRIDGE} PRE_BUILD
    COMMAND cd ${ENCLAVE_DIR}/src && ${SGX_EDGER8R} --trusted ${EDL_FILES_PATH}/Enclave.edl --search-path ${EDL_FILES_PATH} --search-path ${SGX_SDK}/include && mv Enclave_t.h ../include/
    COMMENT "generate trust briage functions"
)

set(Crypto_Library_Name sgx_tcrypto)
file(GLOB_RECURSE Enclave_Cpp_Files "*.cpp")
file(GLOB_RECURSE Enclave_C_Files "*.c")

if(${SGX_DNNL} EQUAL 0)
    list(FILTER Enclave_Cpp_Files EXCLUDE REGEX "dnnl_*")
endif()

include_directories(${PROJECT_SOURCE_DIR}/common ${ENCLAVE_DIR}/include ${SGX_SDK}/include ${SGX_SDK}/include/tlibc ${SGX_SDK}/include/libcxx)

set(LIBRARY "-lsgx_tstdc -lsgx_tcxx -lsgx_pthread -l${Crypto_Library_Name} -l${Service_Library_Name}")

if(${SGX_DNNL} EQUAL 1)
    add_definitions(-D DNNL)
    set(LIBRARY "${LIBRARY} -lsgx_omp -lsgx_dnnl")
elseif(${OPENMP} EQUAL 1)
    set(LIBRARY "${LIBRARY} -lsgx_omp")
endif()


set(Enclave_C_Flags "${SGX_COMMON_CFLAGS} -nostdinc -fvisibility=hidden -fpie -ffunction-sections -fdata-sections -fstack-protector-strong")
set(Enclave_Cpp_Flags "-Wno-narrowing ${Enclave_C_Flags} -std=c++11 -nostdinc++")


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${Enclave_C_Flags}")#设置Cmake编译C文件的编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Enclave_Cpp_Flags}")#设置CMake编译Cpp文件的编译选项
LINK_DIRECTORIES(${SGX_LIBRARY_PATH})#共享库链接其他库的时候的搜索路径，此处是SDK中提供的静态库


ADD_LIBRARY(${Enclave_Name} SHARED ${Enclave_C_Files} ${Enclave_Cpp_Files} ${TRUSTED_BRIDGE})#生成libenclave.so
TARGET_LINK_LIBRARIES(${Enclave_Name}  
    "-Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L${SGX_LIBRARY_PATH} \
    -Wl,--whole-archive -l${Trts_Library_Name} -Wl,--no-whole-archive \
    -Wl,--start-group ${LIBRARY} -Wl,--end-group    \
    -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
    -Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
    -Wl,--defsym,__ImageBase=0 -Wl,--gc-sections   \
    -Wl,--version-script=${ENCLAVE_DIR}/Enclave.lds") #链接libenclave.so需要的库

ADD_CUSTOM_COMMAND(OUTPUT ${Signed_enclave_name} POST_BUILD
    DEPENDS ${Enclave_Name}
    COMMAND nohup ${SGX_ENCLAVE_SIGNER} sign -key ${CMAKE_CURRENT_SOURCE_DIR}/Enclave_private.pem -enclave ${LIBRARY_OUTPUT_PATH}/libenclave.so -out ${LIBRARY_OUTPUT_PATH}/enclave.signed.so -config ${PROJECT_SOURCE_DIR}/Enclave/Enclave.config.xml >> /dev/null
    COMMENT "sign the enclave.so => ${Signed_enclave_name}"
)
ADD_CUSTOM_TARGET(enclave.sgined.so ALL
    DEPENDS ${Signed_enclave_name}
)
