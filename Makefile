BUILD_DIR = ./build
DEST_DIR = ./build
C_EXE_NAME = client.bin

ARGS := train ../data/mnist.cfg ../cfg/mynet.cfg
CARGS =  train -c cfg/client.cfg # /data/lz/sgx_project_v2/data/train.cfg /data/lz/sgx_project_v2/cfg/e_mynet.cfg

S_EXE_NAME = server.bin
SARGS = -c cfg/server.cfg 
all: build
	cd $(BUILD_DIR) &&  make -j1
runc:
	$(DEST_DIR)/$(C_EXE_NAME) $(CARGS)


runs:
	$(DEST_DIR)/$(S_EXE_NAME) $(SARGS)

run:
	cd $(DEST_DIR) && ./app $(ARGS)

clean:
	rm -rf $(BUILD_DIR) 
	rm -rf App/src/Enclave_u.c App/include/Enclave_u.h Enclave/src/Enclave_t.c Enclave/include/Enclave_t.h
build:
	mkdir $(BUILD_DIR);cd $(BUILD_DIR); cmake ..
