######## SGX SDK Settings fixed ########
OPENMP ?= 1
SGX_SDK ?= /root/sgxsdk
SGX_MODE ?= SIM
SGX_ARCH ?= x64
SGX_DEBUG ?= 1
SGX_DNNL ?= 0
########                        ########


ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
endif
ifeq ($(SGX_ARCH), x86)
	SGX_COMMON_FLAGS := -m32
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x86/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_COMMON_FLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

ifeq ($(SGX_DEBUG), 1)
ifeq ($(SGX_PRERELEASE), 1)
$(error Cannot set SGX_DEBUG and SGX_PRERELEASE at the same time!!)
endif
endif

ifeq ($(SGX_DEBUG), 1)
	SGX_COMMON_FLAGS += -O0 -g
else
	SGX_COMMON_FLAGS += -Ofast
endif



ifeq ($(SGX_DNNL), 1)
	MACRO += -DDNNL
endif
ifeq ($(OPENMP), 1)
SGX_COMMON_FLAGS += -fopenmp 
endif
#-Wall -Wextra：显示警告
#
SGX_COMMON_FLAGS += $(MACRO) -Winit-self -Wpointer-arith -Wreturn-type \
                    -Waddress -Wsequence-point -Wformat-security \
                    -Wmissing-include-dirs -Wfloat-equal -Wundef -Wshadow \
                    -Wcast-align  -Wredundant-decls  
SGX_COMMON_CFLAGS := $(SGX_COMMON_FLAGS) -Wjump-misses-init -Wstrict-prototypes
SGX_COMMON_CXXFLAGS := $(SGX_COMMON_FLAGS) -Wnon-virtual-dtor -std=c++11

######## App Settings ########

ifneq ($(SGX_MODE), HW)
	Urts_Library_Name := sgx_urts_sim
else
	Urts_Library_Name := sgx_urts
endif

APP_SRCDIR := App/src/
APP_OBJDIR := App/obj/


wtf := $(notdir $(wildcard App/src/*.cpp))
#wtf := $(notdir ${wtf})

App_Cpp_Files := $(notdir $(wildcard App/src/*.cpp))
App_Header := $(wildcard ${APP_SRCDIR}include/*.h) Makefile
App_C_Files := $(notdir $(wildcard ${APP_SRCDIR}*.c))

App_Cpp_Objects := $(App_Cpp_Files:.cpp=.o)
App_C_Objects := $(App_C_Files:.c=.o)
#App_C_Objects := gemm.o utils.o cuda.o deconvolutional_layer.o convolutional_layer.o list.o image.o activations.o im2col.o col2im.o blas.o crop_layer.o dropout_layer.o maxpool_layer.o softmax_layer.o data.o matrix.o network.o connected_layer.o cost_layer.o parser.o option_list.o  detection_layer.o captcha.o route_layer.o writing.o box.o nightmare.o normalization_layer.o avgpool_layer.o coco.o dice.o yolo.o detector.o layer.o compare.o regressor.o classifier.o local_layer.o swag.o shortcut_layer.o activation_layer.o rnn_layer.o gru_layer.o rnn.o rnn_vid.o crnn_layer.o demo.o tag.o cifar.o go.o batchnorm_layer.o art.o region_layer.o reorg_layer.o lsd.o super.o voxel.o tree.o


App_Objects := ${App_Cpp_Objects} ${App_C_Objects} 
App_Objects := $(addprefix ${APP_OBJDIR}, ${App_Objects})



App_Include_Paths := -I${APP_SRCDIR} -I${APP_SRCDIR}/include -I$(SGX_SDK)/include 

#-fPIC 产生与位置无关的代码
App_C_Flags := -fPIC -Wno-attributes $(App_Include_Paths) -w

ifeq ($(SGX_DEBUG), 1)
        App_C_Flags += -DDEBUG -UNDEBUG -UEDEBUG
else ifeq ($(SGX_PRERELEASE), 1)
        App_C_Flags += -DNDEBUG -DEDEBUG -UDEBUG
else
        App_C_Flags += -DNDEBUG -UEDEBUG -UDEBUG
endif

App_Cpp_Flags := $(App_C_Flags)
App_Link_Flags := -lpthread  -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) 

ifeq ($(OPENMP), 1)
	App_Link_Flags += -lgomp 
endif

ifneq ($(SGX_MODE), HW)
	App_Link_Flags += -lsgx_uae_service_sim
else
	App_Link_Flags += -lsgx_uae_service
endif

App_Name := application
######## Enclave Settings ########
ifneq ($(SGX_MODE), HW)
	Trts_Library_Name := sgx_trts_sim
	Service_Library_Name := sgx_tservice_sim
else
	Trts_Library_Name := sgx_trts
	Service_Library_Name := sgx_tservice
endif

ENCLAVE_SRCDIR := Enclave/src/
ENCLAVE_OBJDIR := Enclave/obj/


Enclave_Cpp_Files := $(notdir $(wildcard ${ENCLAVE_SRCDIR}*.cpp))
Enclave_C_Files := $(notdir $(wildcard ${ENCLAVE_SRCDIR}*.c))

Enclave_Cpp_Objects := $(Enclave_Cpp_Files:.cpp=.o)
Enclave_C_Objects := $(Enclave_C_Files:.c=.o)

Enclave_Objects := ${Enclave_Cpp_Objects} ${Enclave_C_Objects}
Enclave_Objects := $(addprefix ${ENCLAVE_OBJDIR}, ${Enclave_Objects})

Crypto_Library_Name := sgx_tcrypto
Switchless_Library_Name := sgx_tswitchless

Enclave_Include_Paths :=  -IEnclave/src -IEnclave/src/include  -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/libcxx

Enclave_C_Flags := $(Enclave_Include_Paths) -nostdinc -fvisibility=hidden -fpie -ffunction-sections -fdata-sections -DSGX
CC_BELOW_4_9 := $(shell expr "`$(CC) -dumpversion`" \< "4.9")
ifeq ($(CC_BELOW_4_9), 1)
	Enclave_C_Flags += -fstack-protector
else
	Enclave_C_Flags += -fstack-protector-strong
endif
Enclave_Cpp_Flags := $(Enclave_C_Flags) -nostdinc++

# Enable the security flags
Enclave_Security_Link_Flags := -Wl,-z,relro,-z,now,-z,noexecstack

# To generate a proper enclave, it is recommended to follow below guideline to link the trusted libraries:
#    1. Link sgx_trts with the `--whole-archive' and `--no-whole-archive' options,
#       so that the whole content of trts is included in the enclave.
#    2. For other libraries, you just need to pull the required symbols.
#       Use `--start-group' and `--end-group' to link these libraries.
# Do NOT move the libraries linked with `--start-group' and `--end-group' within `--whole-archive' and `--no-whole-archive' options.
# Otherwise, you may get some undesirable errors.

Enclave_Static_Library_Group = -lsgx_tstdc -lsgx_tcxx -lsgx_pthread -l$(Switchless_Library_Name) -l$(Crypto_Library_Name) -l$(Service_Library_Name)  
ifeq ($(SGX_DNNL), 1)
	Enclave_Static_Library_Group += -lsgx_omp -lsgx_dnnl 
else ifeq ($(OPENMP), 1)
	Enclave_Static_Library_Group += -lsgx_omp
endif

Enclave_Link_Flags := $(Enclave_Security_Link_Flags) \
    -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--start-group $(Enclave_Static_Library_Group) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 -Wl,--gc-sections   \
	-Wl,--version-script=Enclave/Enclave.lds




Enclave_Name := enclave.so
Signed_Enclave_Name := enclave.signed.so
Enclave_Config_File := Enclave/Enclave.config.xml

ifeq ($(SGX_MODE), HW)
ifeq ($(SGX_DEBUG), 1)
	Build_Mode = HW_DEBUG
else ifeq ($(SGX_PRERELEASE), 1)
	Build_Mode = HW_PRERELEASE
else
	Build_Mode = HW_RELEASE
endif
else
ifeq ($(SGX_DEBUG), 1)
	Build_Mode = SIM_DEBUG
else ifeq ($(SGX_PRERELEASE), 1)
	Build_Mode = SIM_PRERELEASE
else
	Build_Mode = SIM_RELEASE
endif
endif

.PHONY: all target run
# all: .config_$(Build_Mode)_$(SGX_ARCH) #.config_HW_DEBUG_x86
# 	@$(MAKE) target

ifeq ($(Build_Mode), HW_RELEASE)
all:  $(App_Name) $(Enclave_Name)
	@echo "The project has been built in release hardware mode."
	@echo "Please sign the $(Enclave_Name) first with your signing key before you run the $(App_Name) to launch and access the enclave."
	@echo "To sign the enclave use the command:"
	@echo "   $(SGX_ENCLAVE_SIGNER) sign -key <your key> -enclave $(Enclave_Name) -out <$(Signed_Enclave_Name)> -config $(Enclave_Config_File)"
	@echo "You can also sign the enclave using an external signing tool."
	@echo "To build the project in simulation mode set SGX_MODE=SIM. To build the project in prerelease mode set SGX_PRERELEASE=1 and SGX_MODE=HW."

else
all: $(App_Name) $(Signed_Enclave_Name)
ifeq ($(Build_Mode), HW_DEBUG)
	@echo "The project has been built in debug hardware mode."
else ifeq ($(Build_Mode), SIM_DEBUG)
	@echo "The project has been built in debug simulation mode."
else ifeq ($(Build_Mode), HW_PRERELEASE)
	@echo "The project has been built in pre-release hardware mode."
else ifeq ($(Build_Mode), SIM_PRERELEASE)
	@echo "The project has been built in pre-release simulation mode."
else
	@echo "The project has been built in release simulation mode."
endif

endif

run: all
ifneq ($(Build_Mode), HW_RELEASE)
	@$(CURDIR)/$(App_Name)
	@echo "RUN  =>  $(App_Name) [$(SGX_MODE)|$(SGX_ARCH), OK]"
endif



######## App Objects ########
${APP_SRCDIR}Enclave_u.h: $(SGX_EDGER8R) Enclave/Enclave.edl
	@cd ${APP_SRCDIR} && $(SGX_EDGER8R) --untrusted ../../Enclave/Enclave.edl --search-path ../../Enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

${APP_SRCDIR}Enclave_u.c: ${APP_SRCDIR}Enclave_u.h


${APP_OBJDIR}%.o: ${APP_SRCDIR}%.cpp ${APP_SRCDIR}Enclave_u.h ${App_Header}
	@echo "generating $@ using  $<"
	@$(CXX) $(SGX_COMMON_CXXFLAGS) $(App_Cpp_Flags) -c $< -o $@
	

${APP_OBJDIR}%.o: ${APP_SRCDIR}%.c ${APP_SRCDIR}Enclave_u.h ${App_Header}
	@echo "generating $@ using  $<"
	@$(CC) $(SGX_COMMON_CFLAGS) $(App_C_Flags) -c $< -o $@



$(App_Name): $(App_Objects)  ${APP_OBJDIR}Enclave_u.o
	@echo "linking $@ with $(App_Objects)"
	@$(CXX) $^ -o $@ $(App_Link_Flags)


######## Enclave Objects ########

${ENCLAVE_SRCDIR}Enclave_t.h: $(SGX_EDGER8R) Enclave/Enclave.edl
	@cd ${ENCLAVE_SRCDIR} && $(SGX_EDGER8R) --trusted ../Enclave.edl --search-path ../ --search-path $(SGX_SDK)/include --search-path src/include
	@echo "generated $@ by sgx_edger8r"

${ENCLAVE_SRCDIR}Enclave_t.c: ${ENCLAVE_SRCDIR}Enclave_t.h



${ENCLAVE_OBJDIR}%.o: ${ENCLAVE_SRCDIR}%.cpp ${ENCLAVE_SRCDIR}Enclave_t.h
	@echo "generated  $@ using $<"
	@$(CXX) $(SGX_COMMON_CXXFLAGS) $(Enclave_Cpp_Flags) -c $< -o $@


${ENCLAVE_OBJDIR}%.o: ${ENCLAVE_SRCDIR}%.c ${ENCLAVE_SRCDIR}Enclave_t.h
	@echo "generated  $@ using $<"
	@$(CC) $(SGX_COMMON_CFLAGS) $(Enclave_C_Flags) -c $< -o $@


$(Enclave_Name): $(Enclave_Objects)  ${ENCLAVE_OBJDIR}Enclave_t.o
	@$(CXX) $^ -o $@ $(Enclave_Link_Flags)
	@echo "LINK =>  $@"

$(Signed_Enclave_Name): $(Enclave_Name) $(Enclave_Config_File) Makefile
	@$(SGX_ENCLAVE_SIGNER) sign -key Enclave/Enclave_private.pem -enclave $(Enclave_Name) -out $@ -config $(Enclave_Config_File)
	@echo "SIGN =>  $@"

.PHONY: clean
clean:
	@echo "$(App_Objects)"
	@rm -rf .config_* $(App_Name) $(Enclave_Name) $(Signed_Enclave_Name) $(App_Objects) ${APP_SRCDIR}Enclave_u.* $(Enclave_Objects) ${ENCLAVE_SRCDIR}Enclave_t.*
	@rm -rf build/*
test:
	@echo ${App_Objects}