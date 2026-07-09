#!/bin/sh

set -e

mkdir -p build

gcc -std=c11 -Wall -Wextra -O2 -c ewbd.c -o build/ewbd.o
gcc -std=c11 -Wall -Wextra -O2 -c cgi_parser.c -o build/cgi_parser.o
gcc -std=c11 -Wall -Wextra -O2 -c ewIA.c -o build/ewIA.o
gcc -std=c11 -Wall -Wextra -O2 -c ewb_cgi.c -o build/ewb_cgi.o
g++ -std=c++11 -Wall -Wextra -O2 -c vm_parts.cpp -o build/vm_parts.o
g++ -std=c++11 -Wall -Wextra -O2 -c vm_1.0.cpp -o build/vm_1.0.o
g++ -std=c++11 -Wall -Wextra -O2 -c db_interface.cpp -o build/db_interface.o
gcc -std=c11 -Wall -Wextra -O2 build/ewIA.o -o build/ewIA
g++ -std=c++11 -Wall -Wextra -O2 build/ewbd.o build/cgi_parser.o build/vm_1.0.o build/vm_parts.o build/db_interface.o -o build/ewbd -lmysqlclient -lpq
g++ -std=c++11 -Wall -Wextra -O2 build/ewb_cgi.o build/cgi_parser.o build/vm_1.0.o build/vm_parts.o build/db_interface.o -o build/ewb_cgi -lmysqlclient -lpq
