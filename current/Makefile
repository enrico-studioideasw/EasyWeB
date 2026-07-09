CC=gcc
CXX=g++
CFLAGS=-std=c11 -Wall -Wextra -O2
CXXFLAGS=-std=c++11 -Wall -Wextra -O2
BUILD=build
INSTALL_BIN=/usr/local/bin
SERVICE_DIR=/etc/systemd/system
RESOURCE_INSTALL_DIR=/usr/local/share/ewb/IA-Resources

all: $(BUILD)/ewbd $(BUILD)/ewb_cgi $(BUILD)/ewIA $(BUILD)/cgi_parser.o $(BUILD)/vm_parts.o $(BUILD)/vm_1.0.o $(BUILD)/db_interface.o

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/ewbd.o: ewbd.c cgi_parser.h vm_1.0.h | $(BUILD)
	$(CC) $(CFLAGS) -c ewbd.c -o $(BUILD)/ewbd.o

$(BUILD)/ewbd: $(BUILD)/ewbd.o $(BUILD)/cgi_parser.o $(BUILD)/vm_1.0.o $(BUILD)/vm_parts.o $(BUILD)/db_interface.o
	$(CXX) $(CXXFLAGS) $(BUILD)/ewbd.o $(BUILD)/cgi_parser.o $(BUILD)/vm_1.0.o $(BUILD)/vm_parts.o $(BUILD)/db_interface.o -o $(BUILD)/ewbd -lmysqlclient -lpq

$(BUILD)/cgi_parser.o: cgi_parser.c cgi_parser.h | $(BUILD)
	$(CC) $(CFLAGS) -c cgi_parser.c -o $(BUILD)/cgi_parser.o

$(BUILD)/ewIA.o: ewIA.c | $(BUILD)
	$(CC) $(CFLAGS) -c ewIA.c -o $(BUILD)/ewIA.o

$(BUILD)/ewIA: $(BUILD)/ewIA.o
	$(CC) $(CFLAGS) $(BUILD)/ewIA.o -o $(BUILD)/ewIA

$(BUILD)/ewb_cgi.o: ewb_cgi.c cgi_parser.h vm_1.0.h | $(BUILD)
	$(CC) $(CFLAGS) -c ewb_cgi.c -o $(BUILD)/ewb_cgi.o

$(BUILD)/vm_parts.o: vm_parts.cpp vm_parts.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -c vm_parts.cpp -o $(BUILD)/vm_parts.o

$(BUILD)/vm_1.0.o: vm_1.0.cpp vm_1.0.h vm_parts.h db_interface.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -c vm_1.0.cpp -o $(BUILD)/vm_1.0.o

$(BUILD)/db_interface.o: db_interface.cpp db_interface.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -c db_interface.cpp -o $(BUILD)/db_interface.o

$(BUILD)/ewb_cgi: $(BUILD)/ewb_cgi.o $(BUILD)/cgi_parser.o $(BUILD)/vm_1.0.o $(BUILD)/vm_parts.o $(BUILD)/db_interface.o
	$(CXX) $(CXXFLAGS) $(BUILD)/ewb_cgi.o $(BUILD)/cgi_parser.o $(BUILD)/vm_1.0.o $(BUILD)/vm_parts.o $(BUILD)/db_interface.o -o $(BUILD)/ewb_cgi -lmysqlclient -lpq

install: all
	install -m 0755 $(BUILD)/ewbd $(INSTALL_BIN)/ewbd
	install -m 0755 $(BUILD)/ewb_cgi $(INSTALL_BIN)/ewb_cgi
	install -m 0755 $(BUILD)/ewIA $(INSTALL_BIN)/ewIA
	install -d $(RESOURCE_INSTALL_DIR)
	install -m 0644 IA-Resources/*.txt $(RESOURCE_INSTALL_DIR)/

install-service: install
	install -m 0644 ewbd.service $(SERVICE_DIR)/ewbd.service
	systemctl daemon-reload
	systemctl enable ewbd.service
	systemctl restart ewbd.service

test: all

clean:
	rm -rf $(BUILD)

.PHONY: all install install-service test clean
