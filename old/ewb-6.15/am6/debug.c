/////////////////////Core_di_linguaggio_amethyst/////////////////////////

/* 
   Questo e' l' interprete del linguaggio puro, versione 6.0.
   Questo modulo e' scritto per debugging sulla macchina virtuale. 
   Traduce il codice della macchina virtuale in asm e lo scrive sullo 
   standard output. Legge da stdin. 
*/

/////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include "ewb_vm.h"
//#include "revtable.h"

unsigned int pos=-1; 
char desc[10]; 
unsigned char getcc()
{ pos++; char c[2]; 
  c[0]=getchar();
  c[1]=0; strcat(desc, c); 
  return c[0];
};

int relative=0; 
int readvar(int c, int d) 
{ //Devo distringuere indirizzamento diretto o indiretto.
  //uso il primo bit di C, se a 0 l'indirizzamento e' indiretto.
  relative=0;
  if (c & 0x80) return (((c & 0x7F)-32)<<6) + (d-32);
  relative=1; 
  return ((c - 32) << 6) + (d - 32); 
};
int readadr()
{ int a, b, c, d; 
  a=getcc(); b=getcc(); c=getcc(); d=getcc(); 
  int rs= ((b-32)<<14) + ((c-32)<<7) + (d-32);
  if (a=='-') rs=-rs; 
  rs=pos + rs - 4; //Correggo perche' ho gia' letto il jmp 
};

readpar()
{ char c=getcc(); 
  if (c=='[')
  { printf("\""); 
    do
    { c=getcc(); 
      if (c=='\\') { c=getcc(); printf("%c", c); c=' '; }
      else { if (c!=']') printf("%c", c); };
    } while (c!=']'); 
    printf("\"\t"); 
  } else 
  { char f=getcc(); 
    int r=readvar(c, f); 
    if (relative) { printf("[SP-%02X]\t", r); } else 
    printf("[%02X]\t", r);  
  };
};

main(int argc, char** argv)
{ char c; 
  if (argc>1)
  { freopen(argv[1], "r", stdin); //Ridirezione su stdin  
  };

  c=getcc(); 
  while (c!=EOF)
  { int k=0; int fnd=0; 
    for (k=0; k<NUM_ICODES; k++)
    { 
      desc[0]=c; desc[1]=0;  
      if (c==instructions[k][1][0]) 
      { fnd=1; printf("%04X %s\t", pos, instructions[k][0]);
        if (instructions[k][0][2]==0) printf("\t");
  
        if (instructions[k][1][1]=='A') 
        { unsigned int adr=0; 
          adr=readadr(); 
          printf("%04X\t\t\t|%s|\n", adr, desc); 
        };
        if (instructions[k][1][1]=='S') 
        { readpar(); unsigned int adr=0; 
          adr=readadr(); 
          printf("%04X\t\t|%s|\n", adr, desc); 
        };

        if (instructions[k][1][1]=='0') printf("\t\t\t|%s|\n", desc); 
        if (instructions[k][1][1]=='1') { readpar(); printf("\t\t|%s|\n", desc); };
        if (instructions[k][1][1]=='2') { readpar(); readpar(); printf("\t|%s|\n", desc); };
//        if (instructions[k][1][1]=='3') { readpar(); readpar(); readpar(); printf("\t(%s)\n", desc); }; 
        k=NUM_ICODES; //Fine giro 
      };
    };
    if (!fnd) printf("%04X ????\n", pos); 
    c=getcc(); 
  };
};
