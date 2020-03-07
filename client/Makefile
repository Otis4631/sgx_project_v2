
App_Name := client
CPP_OBJDIR := obj/
CPP_SRCDIR := src/
App_Include_Paths := -Iinclude/

Cpp_Files := $(notdir $(wildcard src/*.cpp))
Cpp_Header := $(wildcard include/*.h) Makefile

Cpp_Objects := $(Cpp_Files:.cpp=.o)
Cpp_Objects := $(addprefix ${CPP_OBJDIR}, ${Cpp_Objects})
 

Cpp_Flags := -g -Wno-attributes $(App_Include_Paths) -std=c++11
Cpp_Link_Flags := -L/usr/local/lib -lcrypto 



${CPP_OBJDIR}%.o: ${CPP_SRCDIR}%.cpp  ${Cpp_Header}
	$(CXX)  $(Cpp_Flags) -c $< -o $@
	

$(App_Name): $(Cpp_Objects)
	$(CXX) $^ -o $@ $(Cpp_Link_Flags)



.PHONY: all
all: $(App_name)
	@echo "starting"

clean: 
	rm -rf obj/* $(App_Name)