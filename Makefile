CXX=g++
CXXFLAGS=-Wall -std=c++14 -o3 -I./
BUILD_DIR=build
SOURCES=encode.cpp decode.cpp haffman_algorithm.cpp
OBJECTS=$(addprefix $(BUILD_DIR)/, $(SOURCES:.cpp=.o))
EXECUTABLE=hello

all: build_dir_create $(SOURCES) encode decode

encode: encode.o haffman_algorithm.o
	$(CXX) $(CXXFLAGS) $(BUILD_DIR)/encode.o $(BUILD_DIR)/haffman_algorithm.o -o $(BUILD_DIR)/$@

decode: decode.o haffman_algorithm.o
	$(CXX) $(CXXFLAGS) $(BUILD_DIR)/decode.o $(BUILD_DIR)/haffman_algorithm.o -o $(BUILD_DIR)/$@

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $< -o $(BUILD_DIR)/$@

build_dir_create:
	IF not exist $(BUILD_DIR) (mkdir $(BUILD_DIR))

clean:
	del /q $(BUILD_DIR)