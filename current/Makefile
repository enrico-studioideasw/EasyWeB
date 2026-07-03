CC=gcc
CXX=g++
CFLAGS=-std=c11 -Wall -Wextra -O2
CXXFLAGS=-std=c++11 -Wall -Wextra -O2
BUILD=build

all: $(BUILD)/ewbd $(BUILD)/vm_parts.o $(BUILD)/vm_1.0.o $(BUILD)/db_interface.o

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/ewbd: ewbd.c | $(BUILD)
	$(CC) $(CFLAGS) ewbd.c -o $(BUILD)/ewbd

$(BUILD)/vm_parts.o: vm_parts.cpp vm_parts.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -c vm_parts.cpp -o $(BUILD)/vm_parts.o

$(BUILD)/vm_1.0.o: vm_1.0.cpp vm_1.0.h vm_parts.h db_interface.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -c vm_1.0.cpp -o $(BUILD)/vm_1.0.o

$(BUILD)/db_interface.o: db_interface.cpp db_interface.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -c db_interface.cpp -o $(BUILD)/db_interface.o

test: all

clean:
	rm -rf $(BUILD)

.PHONY: all test clean
