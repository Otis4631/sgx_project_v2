BUILD_DIR = ./build
DEST_DIR = ./build
DEST_EXE_NAME = client

all: build
	cd $(BUILD_DIR) &&  make 
run:
	cd $(DEST_DIR) && ./$(DEST_EXE_NAME)
clean:
	rm $(BUILD_DIR) -rf
	rm -rf App/src/Enclave_u.c App/include/Enclave_u.h Enclave/src/Enclave_t.c Enclave/include/Enclave_t.h
build:
	mkdir $(BUILD_DIR);cd $(BUILD_DIR); cmake ..
