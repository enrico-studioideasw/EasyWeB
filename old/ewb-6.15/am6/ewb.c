#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "ewb_vm.h"

main(int argc, char** argv)
{ if ((argc != 3) && (argc != 4)) //per ora rigido: inputfile, outputfile
  { printf ("Uso %s [-d] inputfile.ewb outputfile.cgi\n", argv[0]); 
    exit(-1); 
  };
  int dpos=0; 
  if (!strcmp(argv[1], "-d")) { dpos++; add_debug=1; };
  int f=open(argv[1+dpos], O_RDONLY); 
  if (f>0) 
  { code = (char*)malloc(100000);
    res = (unsigned char*)malloc(50000);
    add_predefs(); 
    
    int i=read(f, code, 100000); 
    close(f); printf("Read %i bytes\n", i);
    code[i]=0; 
    programma();
    printf("\nOutput %i bytes\n", respos);  
    f=open(argv[2+dpos], O_WRONLY | O_CREAT | O_TRUNC, 0777); 
    write(f, res, respos); 
    close(f);   
  } else printf("Errore di lettura file origine\n");  
};
