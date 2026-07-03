#!/bin/sh

set -e

mkdir -p build

gcc -std=c11 -Wall -Wextra -O2 ewbd.c -o build/ewbd
g++ -std=c++11 -Wall -Wextra -O2 -c vm_parts.cpp -o build/vm_parts.o
g++ -std=c++11 -Wall -Wextra -O2 -c vm_1.0.cpp -o build/vm_1.0.o
g++ -std=c++11 -Wall -Wextra -O2 -c db_interface.cpp -o build/db_interface.o
