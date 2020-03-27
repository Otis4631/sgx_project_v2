BUILD_DIR = ./build
DEST_DIR = ./build
DEST_EXE_NAME = client.bin
ARGS =  train # /data/lz/sgx_project_v2/data/train.cfg /data/lz/sgx_project_v2/cfg/e_mynet.cfg

all: build
	cd $(BUILD_DIR) &&  make -j20
run:
	$(DEST_DIR)/$(DEST_EXE_NAME) $(ARGS)
clean:
	rm $(BUILD_DIR) -rf
	rm -rf App/src/Enclave_u.c App/include/Enclave_u.h Enclave/src/Enclave_t.c Enclave/include/Enclave_t.h
build:
	mkdir $(BUILD_DIR);cd $(BUILD_DIR); cmake ..
