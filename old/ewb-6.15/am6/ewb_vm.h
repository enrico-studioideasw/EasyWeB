#ifndef EWB_VM
#define EWB_VM
///////////////////////////Globali ambiente//////////////////////////////

extern char variables[2500][200]; extern int nvars; //1 mega complessivo. 
extern char functions[2500][200]; extern int nfuncs; 
extern int arity[256];   //arity della funzione.
extern int funcpos[256]; //L' indirizzo della funzione. 

///////////////////////////////////ASM///////////////////////////////////
////////////////////////////Per disassemblare////////////////////////////
//VM con istruzioni asm predefinite, e stringhe lunghe.
 
#define LDA  "q"  //Load accumulator
#define STA  "V"  //Store accumulator
#define JMP  "J"  //Jump
//3
#define JZ   "z"  //Jump if zero       - confronti numerici
#define JN   "n"  //Jump if negative 
#define JZN  "Z"  //Jump if zero or negative 
#define JP   "p"  //Jump if positive 
//7
#define JES  "e"  //Jump if empty string
#define JEQ  "Q"  //Confronto di stringhe - equal 
#define JLT  "T"  //Confronto di stringhe - less than 
#define JLE  "E"  //Confronto di stringhe - less or equal
//11
#define NOP  "."  //Nop  Spero non serva
#define CALL ">"  //Call 
#define RET  "<"  //Ritorno da funzione
#define APOP "a"  //apop var -> fa un pop del valore in  var e 
//15              //torna il risultato in A, cambia var. Valori da 0.
#define AGET "g"  //AGET var pos -> torna il valore alla pos di array.
#define POP  "P"  //Pop di accumulatore
#define PUSH "_"  //Push di accumulatore 
#define DSP  "D"  //Decrement stack pointer - push senza toccare A 
#define APUT "i"  //Put var pos Mette quello che c'e' in accumulatore sulla 
//20              //var in posizione pos, trattandola come array.
#define SUM  "+"  //Somma all' accumulatore.    
#define SUB  "-"  //Sottrae dall' accumulatore.    
#define PRD  "*"  //Prodotto con A
#define DIV  "/"  //Divisione A/var
#define MOD  "%"  //Modulo A%var
//25
#define OR   "|"  //or bit a bit
#define LOR  "!"  //or logico 
#define AND  "&"  //and bit a bit
#define LAND "#"  //and logico 
#define NOT  "~"  //negazione logica di A 
//30
#define NEG  "N"  //Negazione di A 
#define PRNT "?"  //print stream A  .. stream e' 1, 2 o un file 
#define SCAT "^"  //Concatena una str all accumulatore
#define DBG  "h"  //Ignorata in esecuzione - inserisce una stringa di debug
//34
#define JEX  "A"  //Eccezione, true o false dopo inp, open, etc..
#define INP  "I"  //input da canale indicato come parametro ->A, se errore alza EX per JEX
#define CLOS "X"  //Chiude lo stream in A
#define OPN  "O"  //open (A, PAR), ritorna il filedes su A. Par e' "r" o "w" o "a".
#define SEN  "s"  //seno di un angolo, in gradi. L' angolo e' su A, come il risultato. 
#define SQR  "r"  //Radice quadrata. 
#define RND  "R"  //Random tra 0 e 1 -> A 
//41
#define LFT  "l"  // left ([var], len) -> A . Modifica anche la var. originale togliendo il pezzo 
#define CHR  "c"  //Il carattere corrispondente al codice ascii
#define ASC  "C"  //L' ascii corrispondente al carattere
#define APSH "u"  //apush var -> fa il push su A del valore di var. Duale di apop  
//45
////////////////////////////Per disassemblare////////////////////////////
#define NUM_ICODES 45
extern char* instructions[NUM_ICODES][2]; 
/////////////////Accesso interprete di linguaggio puro///////////////////
extern char* code; extern int cpos;   //Da valorizzare con il codice da compilare 
extern unsigned char* res;  extern int respos;  //Da allocare per il codice compilato
extern int add_debug; 
int programma(); 
int calcexp();

void out(char* dt);   //Queste per compilare l'output delle predef. 
void outvar(int v);
void outadr(int adr);
void putadr(int pos, int adr); 
////////////////Adder di ambiente e funzioni predefinite/////////////////
int add_predefs();
int predefs(int nf);
/////////////////////////////////////////////////////////////////////////
#endif
