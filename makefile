INPUT=hello_world.cc
OUTPUT=hello_world

export DYLD_LIBRARY_PATH=ChakraCoreFiles/lib

build:
	@ clang++ $(INPUT) -I ChakraCoreFiles/include -std=c++17 -L ChakraCoreFiles/lib/ -l ChakraCore -o $(OUTPUT)

run:
	@ make build
	@ ./$(OUTPUT)
	@ make clean

clean:
	@ rm $(OUTPUT)
