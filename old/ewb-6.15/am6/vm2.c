#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>

#define VMDBG

#include "ewb_vm.h"
#include "revtable.h"
char* var_aput(int varnum, int pos, char* val);

char clgray[8]= {27,'[','0',';','3','8','m',0};
char clnorm[8]= {27,'[','0',';','3','7','m',0};
char clgrn[8]=  {27,'[','0',';','3','2','m',0};
char clred[8]=  {27,'[','0',';','3','3','m',0};

//Variabili e stack 
char* vars[8192];
char CS[200000]; //La ram di programma
char* code=CS; 

//Registri. Punto lo SP in modo da lasciare delle globali. 
int PC=0; int SP=0; char* A=NULL; int VPOS; int VP0; int VP1; 
int ADR; char* B=NULL; char* C=NULL; //Parametri di istr. assembler
int EXC=0; 

//Stringhe lunghe
char* addchr(char* s, char  b); 
char* addstr(char* s, char* b); 
char* clrstr(char* s);

//Conversione  
float tonum(char* c);

char* dezero(char* c) //Tolgo da un numero con la virgola gli zeri finali
{ int ptpos=-1; int k; 
  for (k=0; k<strlen(c); k++)
  {if (c[k]=='.') ptpos=k; };
  if (ptpos>0) 
  { int ptz=1; 
    for (k=strlen(c)-1; k>ptpos; k--)
    { if (ptz)
      { if (c[k]=='0') { c[k]=0; } else ptz=0; };  
    };
    if (c[ptpos+1]==0) c[ptpos]=0; 
  }; return c; 
};

//Parsing 
int relative=0; 
int readvar(int c, int d) 
{ //Devo distringuere indirizzamento diretto o indiretto.
  //uso il primo bit di C, se a 0 l'indirizzamento e' indiretto.
  relative=0;
  if (c & 0x80) return (((c & 0x7F)-32)<<6) + (d-32);
  relative=1; 
  int ofs = ((c - 32) << 6) + (d - 32);
  return SP - ofs; 
};
int readadr();
char* parse_par();
int ishexdigit(char c);
int hexdigit(char c);

char tohexch(char r)
{ if (r<10) return r + '0'; 
  return 'A' + r - 10; 
};

//Stringhe perl e splitting. 
char* str_aput(char* v, int pos, char* val); 
int str_min(char* a, char* b); 
int str_eq(char* a, char* b); 

char toch(char* c) { return c[0]; }; 

main(int argc, char** argv)
{ int k; for (k=0; k<4096; k++) vars[k]=NULL; 

  int fl=open(argv[1], O_RDONLY); 
  if (fl<0) { fprintf(stderr, "No file\n"); exit(0); };
  
  read(fl, code, 200000); close(fl); PC=0; 
  
  char c; int fnd; 
  do
  { c=code[PC]; 
    int k=0; fnd=0;
    if (c==0) { 
                if (argc>2) printf("End at %04X\n", PC); 
                exit(0);  
              }; 
    PC++;   
    for (k=0; k<NUM_ICODES; k++)
    { if (c==instructions[k][1][0]) 
      { fnd=1;
        if (instructions[k][1][1]=='1') B=parse_par(); 
        if (instructions[k][1][1]=='2') { B=parse_par(); VP0=VPOS; C=parse_par(); VP1=VPOS; };
        if (instructions[k][1][1]=='S') { B=parse_par(); readadr(); };
        if (instructions[k][1][1]=='A') readadr(); 
if (argc>2)
{ printf("     S0[%s] S1[%s] S2[%s] S3[%s]\n", vars[0], vars[1], vars[2], vars[3]); 
  printf("     S4[%s] S5[%s] S6[%s] S7[%s]\n", vars[4], vars[5], vars[6], vars[7]); 
  if (instructions[k][1][1]!='A') { printf("A(%s) B(%s) C(%s) adr[%i]\n",A,  B, C, VPOS); }
  else 
  { if (relative) printf("[SP-%02X] A(%s)\n", ADR+SP);  
    else printf("[%04X] A(%s)\n", ADR, A);
  };
  printf("%s%s ", clgrn, instructions[k][0]); 
  printf("%s\n", clnorm); 
  if (argc>3) { char c; do { c=getchar(); } while (c!='\n'); };
};
//Qui gestione istruzione.. abbastanza semplice. Per ora sono 32.
//Ho in B, C gli eventuali parametri, e in ADR l' indirizzo.
//A e' l' accumulatore, vars le variabili, stack, SP lo stack 
//VPOS e' la posizione della var. corrente, utile per STA 
        if (c==toch(LDA))  //var->A
             { clrstr(A); A=addstr(A, B); }
        else if (c==toch(STA))  //A<-var
             { vars[VPOS]=clrstr(vars[VPOS]); vars[VPOS]=addstr(vars[VPOS],A); }
	else if (c==toch(JMP))  //jump
	     { PC=ADR; }
	else if (c==toch(JZ))   //Salta se zero
	     { if (tonum(A)==0) PC=ADR; }
	else if (c==toch(JN))   //Salta se negativo
	     { if (tonum(A)<0)  PC=ADR; }
	else if (c==toch(JZN))  //Salta se 0 o negativo
	     { if (tonum(A)<=0) PC=ADR; }
	else if (c==toch(JP))   //Salta se positivo
	     { if (tonum(A)>0)  PC=ADR; }
	else if (c==toch(JES))  //Salta se stringa vuota  
	     { 
//printf("A is [%s], jumping to %04X\n", A, ADR);  
                if ((A==NULL) || (A[0]==0)) PC=ADR; } 
	else if (c==toch(JEQ))  //Salta se A=B 
	     { if (str_eq(A, B)) PC=ADR; }
	else if (c==toch(JLT))  //Salta se A<B
             { if (str_min(A, B)) PC=ADR; }  
        else if (c==toch(JLE))  //Salta se A<=B 
	     { if ((str_min(A,B)) || (str_eq(A,B))) PC=ADR; }
        else if (c==toch(NOP)) {}  //.nop.
	else if (c==toch(CALL)) //Chiamata di funzione      
	     { vars[SP]=clrstr(vars[SP]); 
               sprintf(vars[SP], "%i", PC); SP++;
               PC=ADR;  
             }
        else if (c==toch(RET))
             { SP--; PC=tonum(vars[SP]); }
        else if (c==toch(APOP))
             { A=clrstr(A); 
               if (vars[VPOS]!=NULL)
               { int p=0; int vl=0; 
                 while ((ishexdigit(vars[VPOS][p])) && (ishexdigit(vars[VPOS][p+1]))) 
                 { char c; c = hexdigit(vars[VPOS][p]) * 16 + hexdigit(vars[VPOS][p+1]); 
                   p=p+2; A=addchr(A, c);  
                 };
                 if (vars[VPOS][p]==' ') p++; 
                 int k; for(k=0; vars[VPOS][k]!=0; k++) vars[VPOS][k]=vars[VPOS][k+p];
               };
             } 
        else if (c==toch(APSH))
             { char* res=clrstr(NULL); 
               if (vars[VPOS]!=NULL) 
               { int p; for (p=0;vars[VPOS][p]!=0; p++)
                 { unsigned char e=vars[VPOS][p];
                   res=addchr(res, tohexch(e>>4)); 
                   res=addchr(res, tohexch(e & 0x0F)); 
                 };
               }; res=addchr(res, ' '); 
               res=addstr(res, A); 
               free(A); A=res; 
             }
        else if (c==toch(AGET))
             { A=clrstr(A); 
               { int k; int r=0; int pos=tonum(C);  
                 for (k=0; (B[k]!=0); k++)
                 { if (pos==r)
                   { pos=-1; int p=k;  
                     if ((r>0) && (B[k]==' ')) p++;
                     while ((ishexdigit(B[p])) && (ishexdigit(B[p+1]))) 
                     { char c=hexdigit(B[p]) * 16 + hexdigit(B[p+1]); 
                       p=p+2; A=addchr(A, c);   
                     };
                   };
                   if (B[k]==' ') r++; 
                 };
               };
             }
        else if (c==toch(POP))
             { SP--; A=clrstr(A); A=addstr(A, vars[SP]); 
               vars[SP]=clrstr(vars[SP]); }
        else if (c==toch(PUSH))
             { vars[SP]=clrstr(vars[SP]); 
               vars[SP]=addstr(vars[SP], A); SP++; }
        else if (c==toch(DSP))
             { SP--; vars[SP]=clrstr(vars[SP]); }
        else if (c==toch(APUT)) //B e' la variabile, C la posizione 
             { if (C==NULL) C=clrstr(NULL); 
               vars[VP0]=var_aput(VP0, tonum(vars[VP1]), A);
             }
        else if (c==toch(SUM))
             { sprintf(A, "%f", tonum(A) + tonum(B)); 
               dezero(A); 
             }
        else if (c==toch(SUB))
             { sprintf(A, "%f", tonum(A) - tonum(B)); dezero(A); 
             }
        else if (c==toch(PRD))
             { sprintf(A, "%f", tonum(A) * tonum(B)); dezero(A); 
             }
        else if (c==toch(DIV))
             { float r=tonum(B); 
               if (r!=0) { sprintf(A, "%f", tonum(A) / r); dezero(A); } 
               else sprintf(A, "NAN"); 
             }
        else if (c==toch(MOD))
             { int r=(int)tonum(B); 
               if (r!=0) { sprintf(A, "%i", ((int)tonum(A)) % r); } 
               else sprintf(A, "NAN"); 
             }
        else if (c==toch(OR))
             { sprintf(A, "%i", ((int)tonum(A)) |  ((int)tonum(B))); }
        else if (c==toch(LOR))
             { sprintf(A, "%i", ((int)tonum(A)) || ((int)tonum(B))); }
        else if (c==toch(AND))
             { sprintf(A, "%i", ((int)tonum(A)) &  ((int)tonum(B))); }
        else if (c==toch(LAND))
             { sprintf(A, "%i", ((int)tonum(A)) && ((int)tonum(B))); }
        else if (c==toch(NOT))
             { sprintf(A, "%i", ! (int)tonum(A)); }
        else if (c==toch(NEG))
             { sprintf(A, "%f", - tonum(A)); }
	else if (c==toch(PRNT))
             { int stream=tonum(B); 
               if (A!=NULL) write(stream, A, strlen(A)); }
	else if (c==toch(INP))
             { int stream=tonum(B); 
               char c[2]; int n; c[1]=0; A=clrstr(A);
               do
               {  n=read(stream, c, 1); A=addstr(A, c);  
               } while ((n>0) && (c[0]!='\n'));
               EXC=0; if (n<=0) EXC=1; 
             }
        else if (c==toch(OPN))
             { int f=-1; EXC=0;  
               if (B[0]=='r') f=open(A, O_RDONLY); 
               if (B[0]=='w') f=open(A, O_WRONLY|O_CREAT|O_TRUNC, 0666); 
               A=clrstr(A); sprintf(A, "%i", f); if (f<0) EXC=1; 
             }
        else if (c==toch(CLOS))
             { close(tonum(A)); 
             }
        else if (c==toch(JEX))
             { if (EXC) PC=ADR; 
               EXC=0;
             }
        else if (c==toch(SCAT))
             { if (B!=NULL) A=addstr(A, B); } 
        else if (c==toch(SEN))
             { float f=tonum(A); sprintf(A, "%f", sin(f*3.1415926535/180)); 
             }
        else if (c==toch(SQR))
             { float f=tonum(A); sprintf(A, "%f", sqrt(f)); 
             }
        else if (c==toch(RND))
             { float f=((float)rand())/RAND_MAX; 
               A=clrstr(A); sprintf(A, "%f",f); 
             }
        else if (c==toch(LFT))
             { int l=tonum(C); A=clrstr(A); int k;
               for (k=0; (k<l) && (vars[VP0][k]!=0); k++)
                   A=addchr(A, vars[VP0][k]);
               for (l=0; vars[VP0][l]!=0; l++) 
                   vars[VP0][l]=vars[VP0][l+k];  
             }
        else if (c==toch(CHR))
             { int f=tonum(A); sprintf(A, "%c", f); 
             }
        else if (c==toch(ASC))
             { int f=(int)A[0]; sprintf(A, "%i", f); 
             }
        else if ((argc>2) && (c==toch(DBG)))
             {  printf(">> %s%s%s\n", clred, B, clnorm);
             };
      };
    };
    if (!fnd) fprintf(stderr, "Unknown instruction code %02X [%c]\n", (unsigned char)c, c);  
  } while ((fnd) && (c!=EOF)); 
  fprintf(stderr, "\nFlow ended\n"); 
};

//Gestione stringhe lunghe PERL

#define STR_STEP 512
char* addchr(char* s, char  b)
{ if (s==NULL) s=(char*)malloc(STR_STEP); 
  if ( ((strlen(s) +1)% STR_STEP) == 0) s=(char*)realloc(s, strlen(s)+1+STR_STEP); 
  int f=strlen(s); s[f]=b; s[f+1]=0;   
  return s; 
}; 
char* addstr(char* s, char* b)
{ if (s==NULL) s=(char*)malloc(STR_STEP); 
  if (b!=NULL) { int k; for(k=0; b[k]!=0; k++) s=addchr(s, b[k]); };
  return s; 
}; 
char* clrstr(char* s)
{ if (s==NULL) { s=(char*)malloc(STR_STEP); }
     else        s=(char*)realloc(s, STR_STEP);
  s[0]=0; return s;  
};
int readadr()
{ int a, b, c, d; 
  a=(unsigned char)code[PC]; b=(unsigned char)code[PC+1]; 
  c=(unsigned char)code[PC+2]; d=(unsigned char)code[PC+3];
  PC=PC+4; 
  unsigned int rs= ((b-32)<<14) + ((c-32)<<7) + (d-32);
  if (a=='-') rs=-rs; 
  ADR = PC + rs - 5; 
};
char* parse_par()
{ char* pr=NULL;
  VPOS=-1;  
  if (code[PC]=='[') 
  { pr=clrstr(pr);
    char cr;   
    do
    { PC++; cr=code[PC];  
      if (cr=='\\')
      { PC++; cr=code[PC]; 
        if (cr=='n')  pr=addchr(pr, '\n'); 
        if (cr=='t')  pr=addchr(pr, '\t'); 
        if (cr=='\\') pr=addchr(pr, '\\'); 
        if (cr==']')  pr=addchr(pr, ']'); 
        cr=' ';  
      } else 
      { if (cr!=']') pr=addchr(pr, cr);
      };
    } while (cr!=']'); 
    PC++; 
  } else
  { VPOS=readvar(code[PC], code[PC+1]);
    pr=clrstr(pr); 
    pr=addstr(pr,vars[VPOS]);
    PC=PC+2;  
  };
  return pr; 
};
float tonum(char* c)
{ //printf("starttn\n"); 
  float r=0, pos=1; int p=0; int neg=1; 
  if (c[0]=='-') { p=1; neg=-1; };  
  while ((c[p]>='0') && (c[p]<='9'))
  { r=r*10 + c[p]-'0'; p++; 
  }; pos=pos/10; 
  if (c[p]=='.')
  { p++; 
    while ((c[p]>='0') && (c[p]<='9'))
    { r=r + ((float)(c[p]-'0'))*pos; p++; pos=pos/10;  
    }; 
  }; //printf("endtn\n"); 
  return r * neg; 
};

char* var_aput(int v, int pos, char* val)
{ //printf("Aput to var[%i], pos [%i], val \"%s\"\n", v, pos, val); 
  return str_aput(vars[v], pos, val); 
};

char* str_aput(char* v, int pos, char* val)
{ int i=0, k=0; 
  if (v==NULL) v=clrstr(v); 
  while (k<pos)
  { if (v[i]==0) v=addchr(v, ' ');   
    if (v[i]==' ') k++; 
    i++; 
  }; //Ora i e' la posizione dove inserire l'elemento. Gli spazi ci sono
  int d=i; 
  while ((v[d]!=' ') && (v[d]!=0)) d++; 
  char * news = clrstr(NULL); 
  news=addstr(news, v); news[i]=0; //Parte a sinistra 
  int h; char s[10];
  for (h=0; val[h]!=0; h++)
  { sprintf(s, "%02X", (unsigned char)(val[h])); 
    news=addstr(news, s);        //nuovo pezzo
  };
  news=addstr(news, &v[d]); 
  v=(char*)realloc(v, strlen(news)+STR_STEP);
  strcpy(v, news);
  free(news);
  return v;   
};
int str_min(char* a, char* b)
{ int k; for (k=0; (a[k]==b[k]) && (a[k]!=0); k++);
  return (a[k] < b[k]); 
}; 
int str_eq(char* a, char* b) { if ((a!=NULL) && (b!=NULL)) return !strcmp(a, b);  return 0; };

int ishexdigit(char c)
{ return ((c>='0') && (c<='9')) || ((c>='a') && (c<='f')) || ((c>='A') && (c<='F')); 
};
int hexdigit(char c)
{ int res=0; 
  if (c<='9') res= c-'0'; 
  if ((c>='a') && (c<='f')) res=c-'a' + 10; 
  if ((c>='A') && (c<='F')) res=c-'A' + 10; 
  return res; 
};
