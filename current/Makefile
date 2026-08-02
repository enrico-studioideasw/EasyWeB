CC=gcc
CXX=g++
CFLAGS=-std=c11 -Wall -Wextra -O2
CXXFLAGS=-std=c++11 -Wall -Wextra -O2
VM_LIBS=-lcurl -lpng
BUILD=build
INSTALL_BIN=/usr/local/bin
SERVICE_DIR=/etc/systemd/system
RESOURCE_INSTALL_DIR=/usr/local/share/ewb/IA-Resources

all: $(BUILD)/ewb $(BUILD)/ewbd $(BUILD)/ewb_cgi $(BUILD)/ewIA $(BUILD)/cgi_parser.o $(BUILD)/cron_parser.o $(BUILD)/vm_parts.o $(BUILD)/prolog_engine.o $(BUILD)/vm.o $(BUILD)/db_interface.o $(BUILD)/sql_write_parser.o

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/ewb: ewb.c opcodes.h ewb_predef.h | $(BUILD)
	$(CC) -std=gnu11 -Wall -Wextra -Wno-error=implicit-int -Wno-return-mismatch ewb.c -o $(BUILD)/ewb

$(BUILD)/ewbd.o: ewbd.c cgi_parser.h cron_db.h cron_parser.h vm.h | $(BUILD)
	$(CC) $(CFLAGS) -c ewbd.c -o $(BUILD)/ewbd.o

$(BUILD)/ewbd: $(BUILD)/ewbd.o $(BUILD)/cgi_parser.o $(BUILD)/cron_parser.o $(BUILD)/vm.o $(BUILD)/vm_parts.o $(BUILD)/prolog_engine.o $(BUILD)/db_interface.o $(BUILD)/sql_write_parser.o $(BUILD)/ewb_hash.o
	$(CXX) $(CXXFLAGS) $(BUILD)/ewbd.o $(BUILD)/cgi_parser.o $(BUILD)/cron_parser.o $(BUILD)/vm.o $(BUILD)/vm_parts.o $(BUILD)/prolog_engine.o $(BUILD)/db_interface.o $(BUILD)/sql_write_parser.o $(BUILD)/ewb_hash.o -o $(BUILD)/ewbd -lmysqlclient -lpq $(VM_LIBS)

$(BUILD)/cgi_parser.o: cgi_parser.c cgi_parser.h | $(BUILD)
	$(CC) $(CFLAGS) -c cgi_parser.c -o $(BUILD)/cgi_parser.o

$(BUILD)/cron_parser.o: cron_parser.c cron_parser.h | $(BUILD)
	$(CC) $(CFLAGS) -c cron_parser.c -o $(BUILD)/cron_parser.o

$(BUILD)/ewIA.o: ewIA.c | $(BUILD)
	$(CC) $(CFLAGS) -c ewIA.c -o $(BUILD)/ewIA.o

$(BUILD)/ewIA: $(BUILD)/ewIA.o
	$(CC) $(CFLAGS) $(BUILD)/ewIA.o -o $(BUILD)/ewIA

$(BUILD)/ewb_cgi.o: ewb_cgi.c cgi_parser.h vm.h | $(BUILD)
	$(CC) $(CFLAGS) -c ewb_cgi.c -o $(BUILD)/ewb_cgi.o

$(BUILD)/vm_parts.o: vm_parts.cpp vm_parts.h cron_parser.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -c vm_parts.cpp -o $(BUILD)/vm_parts.o

$(BUILD)/prolog_engine.o: prolog_engine.cpp prolog_engine.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -c prolog_engine.cpp -o $(BUILD)/prolog_engine.o

$(BUILD)/vm.o: vm.cpp vm.h vm_parts.h db_interface.h ewb_predef.c ewb_hash.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -c vm.cpp -o $(BUILD)/vm.o

$(BUILD)/ewb_hash.o: ewb_hash.cpp ewb_hash.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -c ewb_hash.cpp -o $(BUILD)/ewb_hash.o

$(BUILD)/db_interface.o: db_interface.cpp db_interface.h sql_write_parser.h vm_parts.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -c db_interface.cpp -o $(BUILD)/db_interface.o

$(BUILD)/sql_write_parser.o: sql_write_parser.cpp sql_write_parser.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -c sql_write_parser.cpp -o $(BUILD)/sql_write_parser.o

$(BUILD)/ewb_cgi: $(BUILD)/ewb_cgi.o $(BUILD)/cgi_parser.o $(BUILD)/cron_parser.o $(BUILD)/vm.o $(BUILD)/vm_parts.o $(BUILD)/prolog_engine.o $(BUILD)/db_interface.o $(BUILD)/sql_write_parser.o $(BUILD)/ewb_hash.o
	$(CXX) $(CXXFLAGS) $(BUILD)/ewb_cgi.o $(BUILD)/cgi_parser.o $(BUILD)/cron_parser.o $(BUILD)/vm.o $(BUILD)/vm_parts.o $(BUILD)/prolog_engine.o $(BUILD)/db_interface.o $(BUILD)/sql_write_parser.o $(BUILD)/ewb_hash.o -o $(BUILD)/ewb_cgi -lmysqlclient -lpq $(VM_LIBS)

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

$(BUILD)/test_hash: tests/test_hash.cpp $(BUILD)/ewb_hash.o
	$(CXX) $(CXXFLAGS) tests/test_hash.cpp $(BUILD)/ewb_hash.o -o $(BUILD)/test_hash

$(BUILD)/ewb_run_file: tests/ewb_run_file.cpp $(BUILD)/vm.o $(BUILD)/vm_parts.o $(BUILD)/prolog_engine.o $(BUILD)/db_interface.o $(BUILD)/sql_write_parser.o $(BUILD)/cron_parser.o $(BUILD)/ewb_hash.o
	$(CXX) $(CXXFLAGS) tests/ewb_run_file.cpp $(BUILD)/vm.o $(BUILD)/vm_parts.o $(BUILD)/prolog_engine.o $(BUILD)/db_interface.o $(BUILD)/sql_write_parser.o $(BUILD)/cron_parser.o $(BUILD)/ewb_hash.o -o $(BUILD)/ewb_run_file -lmysqlclient -lpq $(VM_LIBS)

test: all $(BUILD)/test_hash $(BUILD)/ewb_run_file
	$(BUILD)/test_hash
	$(BUILD)/ewb tests/compiler_smoke.ewb $(BUILD)/compiler_compact.evm >/dev/null
	$(BUILD)/ewb -S tests/compiler_smoke.ewb $(BUILD)/compiler_source.evm >/dev/null
	test "$$(od -An -t u1 -N2 $(BUILD)/compiler_compact.evm)" = "   0   1"
	test "$$(head -c 1 $(BUILD)/compiler_source.evm)" != ""
	test "$$(wc -c < $(BUILD)/compiler_compact.evm)" -lt "$$(wc -c < $(BUILD)/compiler_source.evm)"
	test "$$($(BUILD)/ewb_run_file $(BUILD)/compiler_compact.evm)" = "2"
	test "$$($(BUILD)/ewb_run_file $(BUILD)/compiler_source.evm)" = "2"
	test "$$(REQUEST_METHOD=GET QUERY_STRING= $(BUILD)/ewb_cgi $(BUILD)/compiler_compact.evm)" = "$$(REQUEST_METHOD=GET QUERY_STRING= $(BUILD)/ewb_cgi $(BUILD)/compiler_source.evm)"

clean:
	rm -rf $(BUILD)

.PHONY: all install install-service test clean
