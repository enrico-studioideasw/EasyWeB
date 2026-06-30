CC=gcc
CXX=g++
CFLAGS=-std=c11 -Wall -Wextra -O2
CXXFLAGS=-std=c++11 -Wall -Wextra -O2
BUILD=build

all: $(BUILD)/ewbd $(BUILD)/vm $(BUILD)/test_values

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/ewbd: ewbd.c | $(BUILD)
	$(CC) $(CFLAGS) ewbd.c -o $(BUILD)/ewbd

$(BUILD)/vm: vm.cpp ewb_values.cpp ewb_values.h | $(BUILD)
	$(CXX) $(CXXFLAGS) vm.cpp ewb_values.cpp -o $(BUILD)/vm

$(BUILD)/test_values: test_values.cpp ewb_values.cpp ewb_values.h | $(BUILD)
	$(CXX) $(CXXFLAGS) test_values.cpp ewb_values.cpp -o $(BUILD)/test_values

test: $(BUILD)/test_values
	./$(BUILD)/test_values

clean:
	rm -rf $(BUILD)

.PHONY: all test clean
